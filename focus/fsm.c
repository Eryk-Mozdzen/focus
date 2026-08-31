#include <stddef.h>
#include <stdint.h>

#include "focus/fsm.h"

void focus_fsm_init(focus_fsm_t *fsm,
                    focus_fsm_state_t *states,
                    const uint32_t states_capacity,
                    focus_fsm_transition_t *transitions,
                    const uint32_t transitions_capacity,
                    void *user) {
    fsm->current = NULL;

    fsm->states = states;
    fsm->states_capacity = states_capacity;
    fsm->states_num = 0;

    fsm->transitions = transitions;
    fsm->transitions_capacity = transitions_capacity;
    fsm->transitions_num = 0;

    fsm->user = user;
}

focus_fsm_state_t *focus_fsm_add_state(focus_fsm_t *fsm,
                                       const focus_fsm_callback_t enter,
                                       const focus_fsm_callback_t execute,
                                       const focus_fsm_callback_t exit) {
    if(fsm->states_num < fsm->states_capacity) {
        fsm->states[fsm->states_num].enter = enter;
        fsm->states[fsm->states_num].execute = execute;
        fsm->states[fsm->states_num].exit = exit;
        fsm->states_num++;
        return &fsm->states[fsm->states_num - 1];
    }

    return NULL;
}

void focus_fsm_add_transition(focus_fsm_t *fsm,
                              const focus_fsm_state_t *from,
                              const focus_fsm_state_t *to,
                              const focus_fsm_trigger_t trigger) {
    if(fsm->transitions_num < fsm->transitions_capacity) {
        fsm->transitions[fsm->transitions_num].prev = (focus_fsm_state_t *)from;
        fsm->transitions[fsm->transitions_num].next = (focus_fsm_state_t *)to;
        fsm->transitions[fsm->transitions_num].trigger = trigger;
        fsm->transitions_num++;
    }
}

void focus_fsm_add_transition_begin(focus_fsm_t *fsm,
                                    const focus_fsm_state_t *from,
                                    const focus_fsm_trigger_t trigger) {
    if(fsm->transitions_num < fsm->transitions_capacity) {
        fsm->transitions[fsm->transitions_num].prev = (focus_fsm_state_t *)from;
        fsm->transitions[fsm->transitions_num].trigger = trigger;
    }
}

void focus_fsm_add_transition_end(focus_fsm_t *fsm, const focus_fsm_state_t *to) {
    if(fsm->transitions_num < fsm->transitions_capacity) {
        fsm->transitions[fsm->transitions_num].next = (focus_fsm_state_t *)to;
        fsm->transitions_num++;
    }
}

void focus_fsm_start(focus_fsm_t *fsm, const focus_fsm_state_t *initial) {
    fsm->current = (focus_fsm_state_t *)initial;

    if(fsm->current->enter) {
        fsm->current->enter(fsm->user);
    }
}

void focus_fsm_update(focus_fsm_t *fsm) {
    focus_fsm_transition_t *transition;

    do {
        transition = NULL;

        for(uint32_t i = 0; i < fsm->transitions_num; i++) {
            if(fsm->current != fsm->transitions[i].prev) {
                continue;
            }

            if(fsm->transitions[i].trigger) {
                if(fsm->transitions[i].trigger(fsm->user)) {
                    transition = &fsm->transitions[i];
                    break;
                }
            } else {
                transition = &fsm->transitions[i];
                break;
            }
        }

        if(transition) {
            if(fsm->current->exit) {
                fsm->current->exit(fsm->user);
            }

            fsm->current = transition->next;

            if(fsm->current->enter) {
                fsm->current->enter(fsm->user);
            }
        }
    } while(transition);
}

void focus_fsm_execute(focus_fsm_t *fsm) {
    if(fsm->current->execute) {
        fsm->current->execute(fsm->user);
    }
}
