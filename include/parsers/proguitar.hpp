#pragma once
#include "state/base.hpp"
class ProGuitarParser {
   public:
    template <typename T>
    static void updateProGuitar(T* report, san_base_t* data) {
        data->pro_guitar.lowEFretVelocity = report->lowEFretVelocity;
        data->pro_guitar.aFretVelocity = report->aFretVelocity;
        data->pro_guitar.dFretVelocity = report->dFretVelocity;
        data->pro_guitar.gFretVelocity = report->gFretVelocity;
        data->pro_guitar.bFretVelocity = report->bFretVelocity;
        data->pro_guitar.highEFretVelocity = report->highEFretVelocity;
        data->gamepad.back = report->back;
        data->gamepad.start = report->start;
        data->gamepad.guide = report->guide;

        data->gamepad.a = report->a;
        data->gamepad.b = report->b;
        data->gamepad.x = report->x;
        data->gamepad.y = report->y;

        data->pro_guitar.green = report->green;
        data->pro_guitar.red = report->red;
        data->pro_guitar.yellow = report->yellow;
        data->pro_guitar.blue = report->blue;
        data->pro_guitar.orange = report->orange;

        data->pro_guitar.lowEFret = report->lowEFret;
        data->pro_guitar.aFret = report->aFret;
        data->pro_guitar.dFret = report->dFret;
        data->pro_guitar.gFret = report->gFret;
        data->pro_guitar.bFret = report->bFret;
        data->pro_guitar.highEFret = report->highEFret;

        data->pro_guitar.tilt = report->tilt;
        data->pro_guitar.pedal = report->pedal;
    }
};