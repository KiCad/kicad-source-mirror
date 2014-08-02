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

    def AddParam(self, section, param, unit, default, hint = ''):
        """
        Add a parameter with some properties.

        TODO: Hints are not supported, as there is as yet nowhere to
        put them in the KiCAD interface
        """

        val = None
        if unit == self.uMM:
            val = pcbnew.FromMM(default)
        elif unit == self.uMils:
            val = pcbnew.FromMils(default)
        elif unit == self.uNatural:
            val = default
        elif unit == self.uBool:
            val = "True" if default else "False" #ugly stringing
        else:
            print "Warning: Unknown unit type: %s" % unit
            return

        if unit in [self.uNatural, self.uBool]:
            param = "*%s" % param #star prefix for natural

        if section not in self.parameters:
            self.parameters[section] = {}

        self.parameters[section][param] = val

    def _PrintParameterTable(self):
        """
        Pretty-print the parameters we have
        """
        for name, section in self.parameters.iteritems():
            print "  %s:" % name

            for key, value in section.iteritems():
                unit = ""
                if (type(value) is int or type(value) is float) and not "*" in key:
                    unit = "mm"

                if "*" in key:
                    key = key[1:]
                else:
                    value = pcbnew.ToMM(value)

                print "    %s: %s%s" % (key, value, unit)

    def _ParametersHaveErrors(self):
        """
        Return true if we discovered errors suring parameter processing
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

        for name, section in self.parameter_errors.iteritems():
            printed_section = False

            for key, value in section.iteritems():
                if value:
                    if not printed_section:
                        print "  %s:" % name

                    print "       %s: %s (have %s)" % (key, value,
                                        self.parameters[name][key])

    def ProcessParameters(self):
        """
        Make sure the parameters we have meet whatever expectations the
        footprint wizard has of them
        """

        self.ClearErrors()
        self.CheckParameters();

        if self._ParametersHaveErrors():
            print "Cannot build footprint: Parameters have errors:"
            self._PrintParameterErrors()
            return False

        print "Building new %s footprint with the following parameters:" % self.name

        self._PrintParameterTable()
        return True

    #################################################################
    # PARAMETER CHECKERS
    #################################################################

    def CheckParamPositiveInt(self, section, param, min_value = 1,
                                max_value = None, is_multiple_of = 1):
        """
        Make sure a parameter can be made into an int, and enforce
        limits if required
        """

        try:
            self.parameters[section][param] = int(self.parameters[section][param])
        except ValueError:
            self.parameter_errors[section][param] = "Must be a valid integer"
            return

        if min_value is not None and (self.parameters[section][param] < min_value):
            self.parameter_errors[section][param] = "Must be greater than or equal to %d" % (min_value)
            return

        if max_value is not None and (self.parameters[section][param] > min_value):
            self.parameter_errors[section][param] = "Must be less than or equal to %d" % (max_value)
            return

        if is_multiple_of > 1 and (self.parameters[section][param] % is_multiple_of) > 0:
            self.parameter_errors[section][param] = "Must be a multiple of %d" % is_multiple_of
            return

        return

    def CheckParamBool(self, section, param):
        """
        Make sure a parameter looks like a boolean, convert to native
        boolean type if so
        """
        if str(self.parameters[section][param]).lower() in ["true", "t", "y", "yes", "on", "1", "1.0"]:
            self.parameters[section][param] = True;
            return
        elif str(self.parameters[section][param]).lower() in ["false", "f", "n", "no", "off", "0", "0.0"]:
            self.parameters[section][param] = False;
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

    def GetReference(self):
        raise NotImplementedError

    def GetValuePrefix(self):
        return "U" # footprints needing wizards of often ICs

    def GetImage(self):
        return ""

    def BuildThisFootprint(self):
        raise NotImplementedError

    def BuildFootprint(self):
        """
        Actually make the footprint. We defer all but the setup to
        the implmenting class
        """

        if not self.ProcessParameters():
            return

        self.module = pcbnew.MODULE(None) # create a new module

        self.draw = FootprintWizardDrawingAids.FootprintWizardDrawingAids(self.module)

        self.module.SetReference(self.GetReference())
        self.module.SetValue("%s**" % self.GetValuePrefix())

        fpid = pcbnew.FPID(self.module.GetReference())   #the name in library
        self.module.SetFPID( fpid )

        self.BuildThisFootprint() # implementer's build function
