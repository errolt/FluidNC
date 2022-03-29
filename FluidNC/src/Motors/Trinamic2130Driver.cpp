// Copyright (c) 2020 -	Bart Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

/*
    This is used for Trinamic SPI controlled stepper motor drivers.
*/

#include "Trinamic2130Driver.h"

#include "../Machine/MachineConfig.h"

#include <atomic>

namespace MotorDrivers {

    void TMC2130Driver::init() {
        uint8_t cs_id;
        cs_id   = setupSPI();
        tmc2130 = new TMC2130Stepper(cs_id, _r_sense, _spi_index);  // TODO hardwired to non daisy chain index

        // use slower speed if I2S
        if (_cs_pin.capabilities().has(Pin::Capabilities::I2S)) {
            tmc2130->setSPISpeed(_spi_freq);
        }
        TrinamicSpiDriver::finalInit();
    }

    void TMC2130Driver::config_motor() {
        tmc2130->begin();
        _has_errors = !test();  // Try communicating with motor. Prints an error if there is a problem.

        init_step_dir_pins();

        if (_has_errors) {
            return;
        }

        // Run and hold current configuration items are in (float) Amps,
        // but the TMCStepper library expresses run current as (uint16_t) mA
        // and hold current as (float) fraction of run current.

        uint16_t run_i_ma = (uint16_t)(_run_current * 1000.0);

        // The TMCStepper library uses the value 0 to mean 1x microstepping
        int usteps = _microsteps == 1 ? 0 : _microsteps;
        tmc2130->microsteps(usteps);
        tmc2130->rms_current(run_i_ma, TrinamicSpiDriver::holdPercent());

        set_mode(false);
    }

    bool TMC2130Driver::test() { return TrinamicSpiDriver::reportTest(tmc2130->test_connection()); }

    void TMC2130Driver::set_mode(bool isHoming) {
        if (_has_errors) {
            return;
        }

        _mode = static_cast<TrinamicMode>(trinamicModes[isHoming ? _homing_mode : _run_mode].value);

        switch (_mode) {
            case TrinamicMode ::StealthChop:
                log_debug("StealthChop");
                tmc2130->en_pwm_mode(true);
                tmc2130->pwm_autoscale(true);
                tmc2130->diag1_stall(false);
                break;
            case TrinamicMode ::CoolStep:
                log_debug("Coolstep");
                tmc2130->en_pwm_mode(false);
                tmc2130->pwm_autoscale(false);
                tmc2130->TCOOLTHRS(NORMAL_TCOOLTHRS);  // when to turn on coolstep
                tmc2130->THIGH(NORMAL_THIGH);
                break;
            case TrinamicMode ::StallGuard:
                log_debug("Stallguard");
                {
                    auto feedrate = config->_axes->_axis[axis_index()]->_homing->_feedRate;

                    tmc2130->en_pwm_mode(false);
                    tmc2130->pwm_autoscale(false);
                    tmc2130->TCOOLTHRS(calc_tstep(feedrate, 150.0));
                    tmc2130->THIGH(calc_tstep(feedrate, 60.0));
                    tmc2130->sfilt(1);
                    tmc2130->diag1_stall(true);  // stallguard i/o is on diag1
                    tmc2130->sgt(constrain(_stallguard, -64, 63));
                }
                break;
        }
    }

    // Report diagnostic and tuning info
    void TMC2130Driver::debug_message() {
        if (_has_errors) {
            return;
        }

        uint32_t tstep = tmc2130->TSTEP();

        if (tstep == 0xFFFFF || tstep < 1) {  // if axis is not moving return
            return;
        }
        float feedrate = Stepper::get_realtime_rate();  //* settings.microsteps[axis_index] / 60.0 ; // convert mm/min to Hz

        log_info(axisName() << " Stallguard " << tmc2130->stallguard() << "   SG_Val:" << tmc2130->sg_result() << " Rate:" << feedrate
                            << " mm/min SG_Setting:" << constrain(_stallguard, -64, 63));

        // The bit locations differ somewhat between different chips.
        // The layout is the same for TMC2130 and TMC5160
        // TMC2130_n ::DRV_STATUS_t status { 0 };  // a useful struct to access the bits.
        // status.sr = tmc2130 ? tmc2130->DRV_STATUS() : tmc5160->DRV_STATUS();

        // these only report if there is a fault condition
        // report_open_load(status.ola, status.olb);
        // report_short_to_ground(status.s2ga, status.s2gb);
        // report_over_temp(status.ot, status.otpw);
        // report_short_to_ps(bits_are_true(status.sr, 12), bits_are_true(status.sr, 13));

        // log_info(axisName() << " Status Register " << String(status.sr, HEX) << " GSTAT " << String(tmc2130 ? tmc2130->GSTAT() : tmc5160->GSTAT(), HEX));
    }

    void TMC2130Driver::set_disable(bool disable) {
        if (TrinamicSpiDriver::startDisable(disable)) {
            if (_use_enable) {
                tmc2130->toff(TrinamicSpiDriver::toffValue());
            }
        }
    }

    // Configuration registration
    namespace {
        MotorFactory::InstanceBuilder<TMC2130Driver> registration("tmc_2130");
    }
}
