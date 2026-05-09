#ifndef FOCUS_FSM_H
#define FOCUS_FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*focus_fsm_callback_t)(void *);
typedef bool (*focus_fsm_trigger_t)(const void *);
typedef uint32_t focus_fsm_id_t;

struct focus_fsm_state;

typedef struct {
    focus_fsm_trigger_t trigger;
    focus_fsm_callback_t action;
    struct focus_fsm_state *prev;
    struct focus_fsm_state *next;
} focus_fsm_transition_t;

typedef struct focus_fsm_state {
    focus_fsm_id_t id;
    focus_fsm_callback_t enter;
    focus_fsm_callback_t execute;
    focus_fsm_callback_t exit;
} focus_fsm_state_t;

typedef struct {
    focus_fsm_state_t *current;
    focus_fsm_transition_t *transitions;
    uint32_t transitions_capacity;
    uint32_t transitions_num;
    focus_fsm_state_t *states;
    uint32_t states_capacity;
    uint32_t states_num;
    void *user;
} focus_fsm_t;

void focus_fsm_init(focus_fsm_t *fsm,
                    focus_fsm_state_t *states,
                    const uint32_t states_capacity,
                    focus_fsm_transition_t *transitions,
                    const uint32_t transitions_capacity,
                    void *user);
void focus_fsm_add_state(focus_fsm_t *fsm,
                         const focus_fsm_id_t id,
                         const focus_fsm_callback_t enter,
                         const focus_fsm_callback_t execute,
                         const focus_fsm_callback_t exit);
void focus_fsm_add_transition(focus_fsm_t *fsm,
                              const focus_fsm_id_t from,
                              const focus_fsm_id_t to,
                              const focus_fsm_trigger_t trigger,
                              const focus_fsm_callback_t action);
void focus_fsm_start(focus_fsm_t *fsm, const focus_fsm_id_t initial);
void focus_fsm_update(focus_fsm_t *fsm);
void focus_fsm_execute(focus_fsm_t *fsm);

#endif
