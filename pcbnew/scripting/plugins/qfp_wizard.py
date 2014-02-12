import pcbnew

def abs(x):
    if x < 0:
        return -x

    return x

class QFPWizard(pcbnew.FootprintWizardPlugin):
    def __init__(self):
        pcbnew.FootprintWizardPlugin.__init__(self)
        self.name = "QFP"
        self.description = "QFP Footprint Wizard"
        self.parameters = {
            "Pads": {
                "*n":               100,
                "pitch":            pcbnew.FromMM(0.5),
                "width":            pcbnew.FromMM(0.25),
                "length":           pcbnew.FromMM(1.5),
                "horizontal pitch": pcbnew.FromMM(15),
                "vertical pitch":   pcbnew.FromMM(15),
                "*oval":            "True"
            },
            "Package": {
                "width":            pcbnew.FromMM(14),
                "height":           pcbnew.FromMM(14)
            }
        }

        self.ClearErrors()

    def smd_rect_pad(self, module, size, pos, name):
        pad = pcbnew.D_PAD(module)

        pad.SetSize(size)

        if self.parameters['Pads'].get('*oval', "true").lower() == "true":
            pad.SetShape(pcbnew.PAD_OVAL)
        else:
            pad.SetShape(pcbnew.PAD_RECT)

        pad.SetAttribute(pcbnew.PAD_SMD)
        pad.SetLayerMask(pcbnew.PAD_SMD_DEFAULT_LAYERS)
        pad.SetPos0(pos)
        pad.SetPosition(pos)
        pad.SetPadName(name)

        return pad

    def CheckParameters(self):
        errors = ""
        pads = self.parameters

        num_pads = pads["Pads"]["*n"]
        if (num_pads < 1):
            self.parameter_errors["Pads"]["*n"] = "Must be positive"
            errors +="Pads/n has wrong value, "
        pads["Pads"]["*n"] = int(num_pads) # Reset to int instead of float

        return errors

    def BuildFootprint(self):
        if self.has_errors():
            print "Cannot build footprint: Parameters have errors:"
            print self.parameter_errors
            return

        print "Building new QFP footprint with the following parameters:"
        self.print_parameter_table()

        self.module = pcbnew.MODULE(None) # create a new module

        pads = self.parameters
        num_pads = int(pads["Pads"]["*n"])
        pad_width = pads["Pads"]["width"]
        pad_length = pads["Pads"]["length"]
        pad_pitch = pads["Pads"]["pitch"]
        pad_horizontal_pitch = pads["Pads"]["horizontal pitch"]
        pad_vertical_pitch = pads["Pads"]["vertical pitch"]

        package_width = pads["Package"]["width"]
        package_height = pads["Package"]["height"]

        side_length = pad_pitch * ((num_pads / 4) - 1)

        offsetX = pad_pitch * ((num_pads / 4) - 1) / 2
        text_size = pcbnew.wxSize(pcbnew.FromMM(0.8), pcbnew.FromMM(0.8))

        self.module.SetReference("QFP %d" % int(num_pads))
        self.module.Reference().SetPos0(pcbnew.wxPoint(0, pcbnew.FromMM(-0.8)))
        self.module.Reference().SetTextPosition(self.module.Reference().GetPos0())
        self.module.Reference().SetSize(text_size)

        self.module.SetValue("U**")
        self.module.Value().SetPos0(pcbnew.wxPoint(0, pcbnew.FromMM(+0.8)))
        self.module.Value().SetTextPosition(self.module.Value().GetPos0())
        self.module.Value().SetSize(text_size)

        fpid = pcbnew.FPID(self.module.GetReference())   #the name in library
        self.module.SetFPID( fpid )

        pad_size_left_right = pcbnew.wxSize(pad_length, pad_width)
        pad_size_bottom_top = pcbnew.wxSize(pad_width, pad_length)

        for cur_pad in range(0, num_pads):
            side = int(cur_pad / (num_pads / 4)) # 0 -> left, 1 -> bottom, 2 -> right, 3 -> top

            if side == 0 or side == 2:
                pad_size = pad_size_left_right

                pad_pos_x = -(pad_horizontal_pitch / 2)
                pad_pos_y = (cur_pad % (num_pads / 4)) * pad_pitch - (side_length / 2)

                if side == 2:
                    pad_pos_x = -pad_pos_x
                    pad_pos_y = -pad_pos_y

            else:
                pad_size = pad_size_bottom_top

                pad_pos_x = (cur_pad % (num_pads / 4)) * pad_pitch - (side_length / 2)
                pad_pos_y = -(pad_vertical_pitch / 2)

                if side == 1:
                    pad_pos_y = -pad_pos_y
                else:
                    pad_pos_x = -pad_pos_x

            pad_pos = pcbnew.wxPoint(pad_pos_x, pad_pos_y)

            pad = self.smd_rect_pad(self.module, pad_size, pad_pos, str(cur_pad + 1))

            self.module.Add(pad)

        half_package_width = package_width / 2
        half_package_height = package_height / 2

        package_pad_height_offset = abs(package_height - side_length) / 2 - pad_pitch
        package_pad_width_offset = abs(package_width - side_length) / 2 - pad_pitch

        # Bottom Left Edge, vertical line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(-half_package_width, half_package_height - package_pad_height_offset)
        end = pcbnew.wxPoint(-half_package_width, half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Bottom Left Edge, horizontal line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(-half_package_width, half_package_height)
        end = pcbnew.wxPoint(-half_package_width + package_pad_width_offset, half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Bottom Right Edge, vertical line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(half_package_width, half_package_height - package_pad_height_offset)
        end = pcbnew.wxPoint(half_package_width, half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Bottom Right Edge, horizontal line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(half_package_width, half_package_height)
        end = pcbnew.wxPoint(half_package_width - package_pad_width_offset, half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Top Right Edge, vertical line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(half_package_width, -half_package_height + package_pad_height_offset)
        end = pcbnew.wxPoint(half_package_width, -half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Top Right Edge, horizontal line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(half_package_width, -half_package_height)
        end = pcbnew.wxPoint(half_package_width - package_pad_width_offset, -half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

        # Top Left Edge, straight line
        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(pcbnew.FromMM(0.2))
        outline.SetLayer(pcbnew.SILKSCREEN_N_FRONT)
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(-half_package_width, -half_package_height + package_pad_height_offset)
        end = pcbnew.wxPoint(-half_package_width + package_pad_width_offset, -half_package_height)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

    def print_parameter_table(self):
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

    def has_errors(self):
        for name, section in self.parameter_errors.iteritems():
            for k, v in section.iteritems():
                if v:
                    return True

        return False

QFPWizard().register()
