#include "s21_fsm.h"

bool fsm_init(fsm_t *fsm, fsm_context_t ctx,
              const fsm_transition_t *transitions, size_t count,
              fsm_state_t start_state) {
  if (!fsm || !transitions || count < 1) return false;

  fsm->ctx = ctx;
  fsm->transitions = transitions;
  fsm->count = count;
  fsm->current = start_state;
  fsm->processing = false;

  return true;
}

void fsm_destroy(fsm_t *fsm) { (void)fsm; }

bool fsm_process_event(fsm_t *fsm, fsm_event_t event) {
  if (!fsm || fsm->processing) return false;

  fsm->processing = true;  //Защита от рекурсивного вызова fsm_process_event
  for (size_t i = 0; i < fsm->count; ++i) {
    const fsm_transition_t *t = &fsm->transitions[i];
    if (t->src == fsm->current && t->event == event) {
      /* выход из текущего состояния */
      if (t->on_exit) t->on_exit(fsm->ctx);
      /* изменить состояние */
      fsm->current = t->dst;
      /* вход в новое состояние */
      if (t->on_enter) t->on_enter(fsm->ctx);
      fsm->processing = false;
      return true;
    }
  }
  fsm->processing = false;

  return false;
}

void fsm_update(fsm_t *fsm) {
  if (!fsm || fsm->processing) return;

  fsm->processing = true;
  for (size_t i = 0; i < fsm->count; ++i) {
    const fsm_transition_t *t = &fsm->transitions[i];
    if (t->src == fsm->current && t->event == FSM_EVENT_NONE) {
      if (t->on_exit) t->on_exit(fsm->ctx);
      fsm->current = t->dst;
      if (t->on_enter) t->on_enter(fsm->ctx);
      break;
    }
  }
  fsm->processing = false;
}