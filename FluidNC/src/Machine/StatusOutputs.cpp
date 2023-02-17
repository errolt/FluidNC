// Copyright (c) 2021 -  Stefan de Bruijn
// Copyright (c) 2021 -  Mitch Bradley
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "StatusOutputs.h"
#include "../Config.h"      // log_*
#include <esp32-hal-cpu.h>  // getApbFrequency()
#include "../System.h"

namespace Machine {
    StatusOutputs::StatusOutputs() {
       
    }
    StatusOutputs::~StatusOutputs() {}

    void StatusOutputs::init() {
        for (int i = 0; i < sizeof(_StatusOutput)/sizeof(Pin); ++i) {
            Pin& pin = _StatusOutput[i];
            const char* name;
            if (pin.defined()) {
                auto        it    = StateName.find((State)i);
                name              = it == StateName.end() ? "<invalid>" : it->second;                
                log_info("Status Output:" << name << " on Pin:" << pin.name());
                pin.setAttr(Pin::Attr::Output);
                pin.off();
            }
        }
    }
    
    bool StatusOutputs::setStatus(State state) {
        if(_CurrentState == state)
        {
            //Nothing to do. Exit
            return true;
        }
        //Turn off current state output.
        Pin& pin_old = _StatusOutput[(uint)_CurrentState];
        if (pin_old.defined()) {
            pin_old.synchronousWrite(false);
        }

        const char* StateNameBefore;
        const char* StateNameAfter;
        auto        it1    = StateName.find(_CurrentState);
        StateNameBefore              = it1 == StateName.end() ? "<invalid>" : it1->second;                
        auto        it2    = StateName.find(state);
        StateNameAfter              = it2 == StateName.end() ? "<invalid>" : it2->second;                
        log_info("Status outputs changing from "<< StateNameBefore <<" to " <<StateNameAfter);
        //Save new state.
        _CurrentState=state;


        //Turn on selected status
        Pin& pin = _StatusOutput[(uint)state];
        if (pin.undefined()) {
            return true;  // It is okay to turn off an undefined pin, for safety
        }
        pin.synchronousWrite(true);
        return true;
    }

    void StatusOutputs::group(Configuration::HandlerBase& handler) {
        handler.item("Idle_pin", _StatusOutput[(uint)State::Idle]);
        handler.item("Alarm_pin", _StatusOutput[(uint)State::Alarm]);
        handler.item("CheckMode_pin", _StatusOutput[(uint)State::CheckMode]);
        handler.item("Homing_pin", _StatusOutput[(uint)State::Homing]);
        handler.item("Cycle_pin", _StatusOutput[(uint)State::Cycle]);
        handler.item("Hold_pin", _StatusOutput[(uint)State::Hold]);
        handler.item("Jog_pin", _StatusOutput[(uint)State::Jog]);
        handler.item("SafetyDoor_pin", _StatusOutput[(uint)State::SafetyDoor]);
        handler.item("Sleep_pin", _StatusOutput[(uint)State::Sleep]);
        handler.item("ConfigAlarm_pin", _StatusOutput[(uint)State::ConfigAlarm]);
    }
}
