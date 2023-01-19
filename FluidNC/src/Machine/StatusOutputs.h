// Copyright (c) 2021 -  Stefan de Bruijn
// Copyright (c) 2021 -  Mitch Bradley
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#pragma once

#include "../Configuration/Configurable.h"
#include "../GCode.h"       // MaxUserDigitalPin MaxUserAnalogPin


namespace Machine {
    class StatusOutputs : public Configuration::Configurable {
    State CurrentState;

    public:
        StatusOutputs();

        Pin _StatusOutput[10];     

        void init();

        void group(Configuration::HandlerBase& handler) override;
        bool setStatus(State state);

        ~StatusOutputs();
    };
}
