#include "focus/inverter.h"
#include "focus/common.h"
#include "focus/log.h"

#define FOCUS_LOG_MODULE FOCUS_LOG_MODULE_SRV_INVERTER

void focus_inverter_driver(struct focus_srv_inverter *srv, const struct focus_event *event) {
    struct focus_inverter *inv = focus_container_of(srv, struct focus_inverter, srv);

    switch(inv->state) {
        case FOCUS_INVERTER_STATE_WAITING_FOR_CALIBRATION: {
            if(event->type == FOCUS_EVENT_TYPE_SRV_CONTROL_CALIBRATE) {
                inv->accumulator_uvw[0] = 0.f;
                inv->accumulator_uvw[1] = 0.f;
                inv->accumulator_uvw[2] = 0.f;
                inv->accumulator_counter = 0;
                inv->state = FOCUS_INVERTER_STATE_CALIBRATION_OFFSET_UVW;
                FOCUS_LOG_INFO("starting offset measurement on UVW phases");
            }
        } break;
        case FOCUS_INVERTER_STATE_CALIBRATION_OFFSET_UVW: {
            if(event->type == FOCUS_EVENT_TYPE_PORT_INVERTER_SAMPLE) {
                const struct focus_event e = {
                    .type = FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_LOOP,
                    .arg.srv_inverter_calibration_loop.pwm[0] = 0.5f,
                    .arg.srv_inverter_calibration_loop.pwm[1] = 0.5f,
                    .arg.srv_inverter_calibration_loop.pwm[2] = 0.5f,
                };
                focus_send_event(inv->srv.port, &e);

                inv->accumulator_uvw[0] += event->arg.port_inverter_sample.current_u;
                inv->accumulator_uvw[1] += event->arg.port_inverter_sample.current_v;
                inv->accumulator_uvw[2] += event->arg.port_inverter_sample.current_w;
                inv->accumulator_counter++;

                if(inv->accumulator_counter >= inv->config.calibration_offset_samples) {
                    inv->config.params.offset[0] =
                        inv->accumulator_uvw[0] / inv->config.calibration_scale_samples;
                    inv->config.params.offset[1] =
                        inv->accumulator_uvw[1] / inv->config.calibration_scale_samples;
                    inv->config.params.offset[2] =
                        inv->accumulator_uvw[2] / inv->config.calibration_scale_samples;

                    FOCUS_LOG_INFO("offset on U phase: %+7.3f A", inv->config.params.offset[0]);
                    FOCUS_LOG_INFO("offset on V phase: %+7.3f A", inv->config.params.offset[1]);
                    FOCUS_LOG_INFO("offset on W phase: %+7.3f A", inv->config.params.offset[2]);

                    inv->accumulator_uvw[0] = 0.f;
                    inv->accumulator_uvw[1] = 0.f;
                    inv->accumulator_uvw[2] = 0.f;
                    inv->accumulator_counter = 0;
                    inv->state = FOCUS_INVERTER_STATE_CALIBRATION_SCALE_U;
                    FOCUS_LOG_INFO("starting scale measurement on U phase");
                }
            }
        } break;
        case FOCUS_INVERTER_STATE_CALIBRATION_SCALE_U: {
            const float inject = inv->config.calibration_scale_voltage /
                                 event->arg.port_inverter_sample.voltage_vbus;

            const struct focus_event e = {
                .type = FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_LOOP,
                .arg.srv_inverter_calibration_loop.pwm[0] = 0.5f + inject,
                .arg.srv_inverter_calibration_loop.pwm[1] = 0.5f,
                .arg.srv_inverter_calibration_loop.pwm[2] = 0.5f,
            };
            focus_send_event(inv->srv.port, &e);

            inv->accumulator_uvw[0] += event->arg.port_inverter_sample.current_u;
            inv->accumulator_counter++;

            if(inv->accumulator_counter >= inv->config.calibration_scale_samples) {
                FOCUS_LOG_DEBUG("current on U phase: %+7.3f A",
                                inv->accumulator_uvw[0] / inv->config.calibration_scale_samples);

                inv->accumulator_counter = 0;
                inv->state = FOCUS_INVERTER_STATE_CALIBRATION_SCALE_V;
                FOCUS_LOG_INFO("starting scale measurement on V phase");
            }
        } break;
        case FOCUS_INVERTER_STATE_CALIBRATION_SCALE_V: {
            const float inject = inv->config.calibration_scale_voltage /
                                 event->arg.port_inverter_sample.voltage_vbus;

            const struct focus_event e = {
                .type = FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_LOOP,
                .arg.srv_inverter_calibration_loop.pwm[0] = 0.5f,
                .arg.srv_inverter_calibration_loop.pwm[1] = 0.5f + inject,
                .arg.srv_inverter_calibration_loop.pwm[2] = 0.5f,
            };
            focus_send_event(inv->srv.port, &e);

            inv->accumulator_uvw[1] += event->arg.port_inverter_sample.current_v;
            inv->accumulator_counter++;

            if(inv->accumulator_counter >= inv->config.calibration_scale_samples) {
                FOCUS_LOG_DEBUG("current on V phase: %+7.3f A",
                                inv->accumulator_uvw[1] / inv->config.calibration_scale_samples);

                inv->accumulator_counter = 0;
                inv->state = FOCUS_INVERTER_STATE_CALIBRATION_SCALE_W;
                FOCUS_LOG_INFO("starting scale measurement on W phase");
            }
        } break;
        case FOCUS_INVERTER_STATE_CALIBRATION_SCALE_W: {
            const float inject = inv->config.calibration_scale_voltage /
                                 event->arg.port_inverter_sample.voltage_vbus;

            const struct focus_event e = {
                .type = FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_LOOP,
                .arg.srv_inverter_calibration_loop.pwm[0] = 0.5f,
                .arg.srv_inverter_calibration_loop.pwm[1] = 0.5f,
                .arg.srv_inverter_calibration_loop.pwm[2] = 0.5f + inject,
            };
            focus_send_event(inv->srv.port, &e);

            inv->accumulator_uvw[2] += event->arg.port_inverter_sample.current_w;
            inv->accumulator_counter++;

            if(inv->accumulator_counter >= inv->config.calibration_scale_samples) {
                FOCUS_LOG_DEBUG("current on W phase: %+7.3f A",
                                inv->accumulator_uvw[2] / inv->config.calibration_scale_samples);

                const float mean_uvw[3] = {
                    inv->accumulator_uvw[0] / inv->config.calibration_scale_samples,
                    inv->accumulator_uvw[1] / inv->config.calibration_scale_samples,
                    inv->accumulator_uvw[2] / inv->config.calibration_scale_samples,
                };

                const float mean = (mean_uvw[0] + mean_uvw[1] + mean_uvw[2]) / 3.f;

                inv->config.params.scale[0] = mean / mean_uvw[0];
                inv->config.params.scale[1] = mean / mean_uvw[1];
                inv->config.params.scale[2] = mean / mean_uvw[2];

                FOCUS_LOG_INFO("scale on U phase: %+7.3f A", inv->config.params.scale[0]);
                FOCUS_LOG_INFO("scale on V phase: %+7.3f A", inv->config.params.scale[1]);
                FOCUS_LOG_INFO("scale on W phase: %+7.3f A", inv->config.params.scale[2]);

                inv->state = FOCUS_INVERTER_STATE_CALIBRATED;

                FOCUS_LOG_INFO("inverter calibration ended");

                const struct focus_event e = {
                    .type = FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_ENDED,
                };
                focus_send_event(inv->srv.control, &e);
            }
        } break;
        case FOCUS_INVERTER_STATE_CALIBRATED: {
            switch(event->type) {
                case FOCUS_EVENT_TYPE_SRV_CONTROL_CALIBRATE: {
                    inv->accumulator_uvw[0] = 0.f;
                    inv->accumulator_uvw[1] = 0.f;
                    inv->accumulator_uvw[2] = 0.f;
                    inv->accumulator_counter = 0;
                    inv->state = FOCUS_INVERTER_STATE_CALIBRATION_OFFSET_UVW;
                    FOCUS_LOG_INFO("starting offset measurement on UVW phases");
                } break;
                case FOCUS_EVENT_TYPE_SRV_CONTROL_LOOP: {

                } break;
                default: {

                } break;
            }
        } break;
    }
}
