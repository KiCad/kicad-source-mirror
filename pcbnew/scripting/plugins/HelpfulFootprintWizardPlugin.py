#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

import pcbnew
import math
import FootprintWizardDrawingAids


class FootprintWizardParameterManager:
    """
    Functions for helpfully managing parameters to a KiCAD Footprint
    Wizard.

    Abstracts away from whatever structure is used by pcbnew's footprint
    wizard class
    """

    def __init__(self):
        self.parameters = {}
        self.GenerateParameterList()

    def GenerateParameterList(self):
        """
        Construct parameters here, or leave out to have no parameters
        """
        pass

    def CheckParameters(self):
        """
        Implement this to make checks on parameter values, filling
        parameter_errors (or using the checker routines)

        Subclasses can implment their own and override the parent
        defaults and add new ones
        """
        pass

    uMM = 1
    uMils = 2
    uNatural = 3
    uBool = 4
    uString = 5

    def AddParam(self, section, param, unit, default, hint=''):
        """
        Add a parameter with some properties.

        TODO: Hints are not supported, as there is as yet nowhere to
        put them in the KiCAD interface
        """
        error = ""
        val = None
        if unit == self.uMM:
            val = pcbnew.FromMM(default)
        elif unit == self.uMils:
            val = pcbnew.FromMils(default)
        elif unit == self.uNatural:
            val = default
        elif unit == self.uString:
            val = str(default)
        elif unit == self.uBool:
            val = "True" if default else "False"  # ugly stringing
        else:
            error = "Warning: Unknown unit type: %s" % unit
            return error

        if unit in [self.uNatural, self.uBool, self.uString]:
            param = "*%s" % param  # star prefix for natural

        if section not in self.parameters:
            if not hasattr(self, 'page_order'):
                self.page_order = []
            self.page_order.append(section)
            self.parameters[section] = {}
            if not hasattr(self, 'parameter_order'):
                self.parameter_order = {}
            self.parameter_order[section] = []

        self.parameters[section][param] = val
        self.parameter_order[section].append(param)

        return error


    def _PrintParameterTable(self):
        """
        Pretty-print the parameters we have
        """
        message = ""

        for name, section in self.parameters.iteritems():
            message += "  %s:\n" % name

            for key, value in section.iteritems():
                unit = ""
                if ((type(value) is int or type(value) is float)
                        and not "*" in key):
                    unit = "mm"

                if "*" in key:
                    key = key[1:]
                else:
                    value = pcbnew.ToMM(value)

                message += "    %s: %s%s\n" % (key, value, unit)

        return message


    def _ParametersHaveErrors(self):
        """
        Return true if we discovered errors during parameter processing
        """

        for name, section in self.parameter_errors.iteritems():
            for k, v in section.iteritems():
                if v:
                    return True

        return False

    def _PrintParameterErrors(self):
        """
        Pretty-print parameters with errors
        """
        errors = ""

        for name, section in self.parameter_errors.iteritems():
            printed_section = False

            for key, value in section.iteritems():
                if value:
                    if not printed_section:
                        errors += "  %s:" % name

                    errors += "       %s: %s (have %s)\n" % (
                        key, value, self.parameters[name][key])

        return errors

    def ProcessParameters(self):
        """
        Make sure the parameters we have meet whatever expectations the
        footprint wizard has of them
        """

        self.ClearErrors()
        self.CheckParameters()

        if self._ParametersHaveErrors():
            return False

        return True

    #################################################################
    # PARAMETER CHECKERS
    #################################################################

    def CheckParamInt(self, section, param, min_value=1,
                      max_value=None, is_multiple_of=1):
        """
        Make sure a parameter can be made into an int, and enforce
        limits if required
        """

        try:
            self.parameters[section][param] = (
                int(self.parameters[section][param]))
        except ValueError:
            self.parameter_errors[section][param] = (
                "Must be a valid integer")
            return

        if min_value is not None and (
                self.parameters[section][param] < min_value):
            self.parameter_errors[section][param] = (
                "Must be greater than or equal to %d" % (min_value))
            return

        if max_value is not None and (
                self.parameters[section][param] > max_value):
            self.parameter_errors[section][param] = (
                "Must be less than or equal to %d" % (max_value))
            return

        if is_multiple_of > 1 and (
                self.parameters[section][param] % is_multiple_of) > 0:
            self.parameter_errors[section][param] = (
                "Must be a multiple of %d" % is_multiple_of)
            return

        return

    def CheckParamBool(self, section, param):
        """
        Make sure a parameter looks like a boolean, convert to native
        boolean type if so
        """
        if str(self.parameters[section][param]).lower() in [
                "true", "t", "y", "yes", "on", "1", "1.0"]:
            self.parameters[section][param] = True
            return
        elif str(self.parameters[section][param]).lower() in [
                "false", "f", "n", "no", "off", "0", "0.0"]:
            self.parameters[section][param] = False
            return

        self.parameter_errors[section][param] = "Must be boolean (true/false)"
        return


class HelpfulFootprintWizardPlugin(pcbnew.FootprintWizardPlugin,
                                   FootprintWizardParameterManager):
    """
    A class to simplify many aspects of footprint creation, leaving only
    the foot-print specific routines to the wizards themselves

    Generally, you need to implement:
        GetReference()
        GetValue()
        GenerateParameterList()
        CheckParameters()
        BuildThisFootprint()
        GetName()
        GetDescription()
    """
    def __init__(self):
        pcbnew.FootprintWizardPlugin.__init__(self)
        FootprintWizardParameterManager.__init__(self)

        self.name = self.GetName()
        self.decription = self.GetDescription()
        self.image = self.GetImage()

    def GetValue(self):
        raise NotImplementedError

    # this value come from our KiCad Library Convention 0.11
    def GetReferencePrefix(self):
        return "REF"

    def GetImage(self):
        return ""

    def GetTextSize(self):
        """
        IPC nominal
        """
        return pcbnew.FromMM(1.2)

    def GetTextThickness(self):
        """
        Thicker than IPC guidelines (10% of text height = 0.12mm)
        as 5 wires/mm is a common silk screen limitation
        """
        return pcbnew.FromMM(0.15)

    def SetModule3DModel(self):
        """
        Set a 3D model for the module

        Default is to do nothing, you need to implement this if you have
        a model to set

        FIXME: This doesn't seem to be enabled yet?
        """
        pass

    def BuildThisFootprint(self):
        """
        Draw the footprint.

        This is specific to each footprint class, you need to implment
        this to draw what you want
        """
        raise NotImplementedError

    def BuildFootprint( self ):
        """
        Actually make the footprint. We defer all but the setup to
        the implementing class
        """

        self.buildmessages = ""

        self.module = pcbnew.MODULE(None)  # create a new module
        # do it first, so if we return early, we don't segfault KiCad

        if not self.ProcessParameters():
            self.buildmessages = "Cannot build footprint: Parameters have errors:\n"
            self.buildmessages += self._PrintParameterErrors()
            return

        self.buildmessages = ("Building new %s footprint with the following parameters:\n"
                   % self.name)

        self.buildmessages += self._PrintParameterTable()

        self.draw = FootprintWizardDrawingAids.FootprintWizardDrawingAids(
            self.module)

        self.module.SetValue(self.GetValue())
        self.module.SetReference("%s**" % self.GetReferencePrefix())

        fpid = pcbnew.FPID(self.module.GetValue())  # the name in library
        self.module.SetFPID(fpid)

        self.SetModule3DModel()  # add a 3d module if specified

        thick = self.GetTextThickness()

        self.module.Reference().SetThickness(thick)
        self.module.Value().SetThickness(thick)

        self.BuildThisFootprint()  # implementer's build function

        return
