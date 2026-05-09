#include <stddef.h>
#include <stdint.h>

#include "focus/fsm.h"

static focus_fsm_state_t *find_state(focus_fsm_t *fsm, const focus_fsm_id_t id) {
    for(uint32_t i = 0; i < fsm->states_num; i++) {
        if(fsm->states[i].id == id) {
            return &fsm->states[i];
        }
    }

    return NULL;
}

void focus_fsm_init(focus_fsm_t *fsm,
                    focus_fsm_state_t *states,
                    const uint32_t states_capacity,
                    focus_fsm_transition_t *transitions,
                    const uint32_t events_capacity,
                    void *user) {
    fsm->current = NULL;

    fsm->states = states;
    fsm->states_capacity = states_capacity;
    fsm->states_num = 0;

    fsm->transitions = transitions;
    fsm->transitions_capacity = events_capacity;
    fsm->transitions_num = 0;

    fsm->user = user;
}

void focus_fsm_add_state(focus_fsm_t *fsm,
                         const focus_fsm_id_t id,
                         const focus_fsm_callback_t enter,
                         const focus_fsm_callback_t execute,
                         const focus_fsm_callback_t exit) {
    if(fsm->states_num < fsm->states_capacity) {
        fsm->states[fsm->states_num].id = id;
        fsm->states[fsm->states_num].enter = enter;
        fsm->states[fsm->states_num].execute = execute;
        fsm->states[fsm->states_num].exit = exit;
        fsm->states_num++;
    }
}

void focus_fsm_add_transition(focus_fsm_t *fsm,
                              const focus_fsm_id_t from,
                              const focus_fsm_id_t to,
                              const focus_fsm_trigger_t trigger,
                              const focus_fsm_callback_t action) {
    focus_fsm_state_t *from_state = find_state(fsm, from);
    focus_fsm_state_t *to_state = find_state(fsm, to);

    if(fsm->transitions_num < fsm->transitions_capacity) {
        fsm->transitions[fsm->transitions_num].trigger = trigger;
        fsm->transitions[fsm->transitions_num].action = action;
        fsm->transitions[fsm->transitions_num].prev = from_state;
        fsm->transitions[fsm->transitions_num].next = to_state;
        fsm->transitions_num++;
    }
}

void focus_fsm_start(focus_fsm_t *fsm, const focus_fsm_id_t initial) {
    focus_fsm_state_t *initial_state = find_state(fsm, initial);

    fsm->current = initial_state;

    if(fsm->current->enter) {
        fsm->current->enter(fsm->user);
    }
}

void focus_fsm_update(focus_fsm_t *fsm) {
    focus_fsm_transition_t *transition = NULL;

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

        if(transition->action) {
            transition->action(fsm->user);
        }

        fsm->current = transition->next;

        if(fsm->current->enter) {
            fsm->current->enter(fsm->user);
        }
    }
}

void focus_fsm_execute(focus_fsm_t *fsm) {
    if(fsm->current->execute) {
        fsm->current->execute(fsm->user);
    }
}
