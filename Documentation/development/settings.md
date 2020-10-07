# Settings Framework

The settings framework manages application settings, as well as projects.  This document explains
how to make use of the framework as well as some of its inner workings.

[TOC]

## Source Code Guide

Most of the relevant code is in `common/settings` and `common/project`.

C++ Class                | Description
:------------------------|:-------------
`SETTINGS_MANAGER`       | Loads and unloads settings files and projects
`JSON_SETTINGS`          | The base class for all settings objects.  Represents a single JSON file.
`NESTED_SETTINGS`        | A `JSON_SETTINGS` object stored within another (i.e. without its own file)
`PARAM`                  | A parameter: helper class for storing data inside a `JSON_SETTINGS`
`APP_SETTINGS_BASE`      | Base class for application (frame) settings
`COLOR_SETTINGS`         | A subclass of `JSON_SETTINGS` designed for storing color themes
`COMMON_SETTINGS`        | The settings available to every part of KiCad
`PROJECT_FILE`           | A `JSON_SETTINGS` representing a project (`.kicad_pro`) file
`PROJECT_LOCAL_SETTINGS` | A `JSON_SETTINGS` representing a project local state (`.kicad_prl`) file

## Where Settings are Stored

There are four main places a setting might be stored:

1) In `COMMON_SETTINGS`: this is a setting that is shared between all parts of KiCad.
2) In an application settings object (subclass of `APP_SETTINGS_BASE`).  These objects, such as
   `EESCHEMA_SETTINGS` and `PCBNEW_SETTINGS`, store settings that are specific to a portion of
   KiCad.  In particular, these objects are compiled inside the context of their respective
   application, so they have access to data types that may not be part of `common`.
3) In the `PROJECT_FILE`, where they will be specific to a loaded project.  This is true of most of
   the settings found in the Board / Schematic Setup dialogs.  Currently, KiCad only supports having
   one project loaded at a time, and a number of places in the code expect that a `PROJECT` object
   will always be available.  Because of this, the `SETTINGS_MANAGER` will always ensure that a
   "dummy" `PROJECT_FILE` is available even when no project has been loaded by the user.  This dummy
   project can be modified in memory but not saved to disk.
4) In the `PROJECT_LOCAL_SETTINGS` object, where they will be specific to a loaded project.  This
   file is for settings that are "local state", such as which board layers are visible, that should
   (for many users, at least) not be checked in to source control.  Any setting here should be
   transient, meaning there will be no ill effect if the entire file is deleted.
   
## JSON_SETTINGS

The `JSON_SETTINGS` class is the backbone of the settings infrastructure.  It is a subclass of the
`nlohmann::json::basic_json` class provided by `thirdparty/nlohmann_json/nlohmann/json.hpp`.  As
such, anytime raw manipulation of the underlying JSON data is needed, you can use the [standard
`nlohmann::json` API](https://nlohmann.github.io/json/api/basic_json/).  The JSON contents represent
the **state of the file on disk**, not the state of the data exposed to C++.  Synchronization
between the two is done via parameters (see below) and takes place right after loading from disk and
right before saving to disk.
   
## Parameters

Parameters establish the link between C++ data and content in the JSON file. In general, parameters
consist of a **path**, **pointer**, and **default value**.  The path is a string of the form
`"x.y.z"`, where each component represents a nested JSON dictionary key.  The pointer is a pointer
to the C++ member variable that holds the data accessible to consumers of the `JSON_SETTINGS`. The
default value is used to update the pointer when the data is missing from the JSON file.

Parameters are subclasses of `PARAM_BASE` in `include/settings/parameters.h`.  There are a number of
helpful subclasses created to make it easier to store complex data in JSON files.

The basic `PARAM` class is templated and is useful for storing any data type can be serialized to
JSON automatically.  A basic instantiation of a `PARAM` might look like:

    m_params.emplace_back( new PARAM<int>( "appearance.icon_scale",
                                           &m_Appearance.icon_scale, 0 ) );
                                           
Here, `m_Appearance.icon_scale` is a public member of the settings object (an `int` inside a
`struct`).  `"appearance.icon_scale"` is the **path** to store the value in the JSON file, and `0`
is the default value.  This would result in JSON looking like this:

    {
        "appearance": {
            "icon_scale": 0
        }
    }

Note that it is possible to use custom types with `PARAM<>` as long as they have a `to_json` and
`from_json` defined.  See `COLOR4D` for an example of this.

For storing complex data types, it is sometimes easiest to use `PARAM_LAMBDA<>`, which allows you
to define a "getter" and "setter" as part of the parameter definition.  You can use this to build
a `nlohmann::json` object and store it as the "value" of your parameter.  For examples of how this
is done, see `NET_SETTINGS`.

## NESTED_SETTINGS

The `NESTED_SETTINGS` class is like a `JSON_SETTINGS` but instead of a file for a backing store, it
uses another `JSON_SETTINGS` object.  The entire contents of a `NESTED_SETTINGS` are stored as the
value of a particular key in the parent file.  This has two key benefits:

1) You can split up large sets of settings in to more manageable pieces
2) You can hide knowledge about the nested settings from the parent settings object

For example, many portions of the project file are stored as `NESTED_SETTINGS` objects inside the
`PROJECT_FILE`.  These objects, including `SCHEMATIC_SETTINGS`, `NET_SETTINGS`, and 
`BOARD_DESIGN_SETTINGS`, are compiled as part of eeschema or pcbnew, so they have access to data
types not available in common (where `PROJECT_FILE` is compiled).

When the outer file is loaded, all of the data for the nested settings is there in the underlying
`nlohmann::json` data store -- it's just not used until the appropriate `NESTED_SETTINGS` is loaded.

`NESTED_SETTINGS` objects can have shorter lifecycles than the parent.  This is required because in
some cases (such as with the project file), the parent can stay resident in one frame (the KiCad
manager, for example) while a frame that uses a nested settings inside it can be created and
destroyed.  When the nested settings is destroyed, it ensures that its data is stored to the JSON
data of the parent.  The parent `JSON_SETTINGS` can then be saved to disk, if desired.

## Schema Version and Migrations

Settings objects have a **schema version**, which is a const integer that can be incremented when a
migration is needed.  The schema version in the code is compared to that in the loaded file, and if
the file version is lower (older), **migrations** are run to bring the data in the file up to date.

**Migrations** are functions that are responsible for making the necessary changes from one schema
version to another.  They act on the **underlying JSON data**, before parameter loading has taken
place.

Migrations are not always needed when changing a settings file.  You are free to add or remove
parameters without changing the schema version or writing migrations.  If you add parameters, they
will be added to the JSON file and initialized to their default value.  If you remove parameters,
they will be silently dropped from the JSON file the next time the settings are saved.  Migration is
only needed when you need to make changes to the JSON file that depend on the current state.  For
example, if you decide to rename a settings key, but want to preserve the user's present setting.

If you need to make a "breaking change" to a settings file:

1) Increment the schema version
2) Write a migration that makes the necessary changes to the underlying `nlohmann::json` object
3) Call `JSON_SETTINGS::registerMigration` in the constructor for the object

