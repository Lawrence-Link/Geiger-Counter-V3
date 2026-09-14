#pragma once

#include "etl/vector.h"
#include "etl/optional.h"
#include "etl/functional.h"
#include "etl/algorithm.h"
#include "etl/mutex.h"
#include <stdint.h>
#include <stdio.h>

/**
 * @brief State Machine namespace.
 */
namespace sm {

/**
 * @brief A generic State Machine implementation.
 *
 * This class provides a simple, table-driven state machine that uses the
 * Embedded Template Library (ETL) containers. It supports states, events,
 * transitions with guards and actions, and state entry/exit callbacks.
 *
 * @tparam StateT The type used to represent states (e.g., an enum class).
 * @tparam EventT The type used to represent events (e.g., an enum class).
 * @tparam ContextT The type of the context/data structure passed to actions and guards.
 * @tparam MaxTransitions The maximum number of transitions the machine can hold.
 * @tparam MaxStates The maximum number of states for which callbacks can be stored.
 */
template <typename StateT, typename EventT, typename ContextT,
          size_t MaxTransitions, size_t MaxStates>
class StateMachine {
public:
    /**
     * @brief Defines a single transition in the state machine.
     */
    struct Transition {
        StateT from; ///< The state the transition starts from.
        EventT event; ///< The event that triggers the transition.
        StateT to; ///< The state the transition moves to.
        etl::ifunction<bool, const ContextT&>* guard = nullptr; ///< Optional function to check before transition; returns true to allow.
        etl::ifunction<void, ContextT&>* action = nullptr; ///< Optional function executed during the transition.
        bool internal = false; ///< If true, it's an internal transition (no exit/entry callbacks are called, state doesn't change).
    };

    /**
     * @brief Defines optional entry and exit callbacks for a state.
     */
    struct StateCallbacks {
        StateT state; ///< The state this callback structure belongs to.
        etl::ifunction<void, ContextT&>* on_entry = nullptr; ///< Optional function executed when entering the state.
        etl::ifunction<void, ContextT&>* on_exit = nullptr; ///< Optional function executed when exiting the state.
    };

    /**
     * @brief Constructs a new StateMachine object.
     */
    StateMachine() : transitions_(), state_callbacks_(), current_() {}

    /**
     * @brief Adds a transition to the state machine's transition table.
     * @param t The transition to add.
     */
    void add_transition(const Transition& t) {
        transitions_.push_back(t);
    }

    /**
     * @brief Sets the entry callback function for a given state.
     * @param s The state to set the entry callback for.
     * @param cb A pointer to the callback function.
     */
    void set_entry(StateT s, etl::ifunction<void, ContextT&>* cb) {
        auto *c = get_or_create_callbacks(s);
        c->on_entry = cb;
    }

    /**
     * @brief Sets the exit callback function for a given state.
     * @param s The state to set the exit callback for.
     * @param cb A pointer to the callback function.
     */
    void set_exit(StateT s, etl::ifunction<void, ContextT&>* cb) {
        auto *c = get_or_create_callbacks(s);
        c->on_exit = cb;
    }

    /**
     * @brief Initializes and starts the state machine at the specified initial state.
     *
     * Calls the `on_entry` callback for the initial state if one is set.
     *
     * @param initial The starting state.
     * @param ctx The context object passed to the `on_entry` callback.
     */
    void start(StateT initial, ContextT& ctx) {
        current_.emplace(initial);
        if (auto *cb = find_callbacks(initial); cb && cb->on_entry) (*cb->on_entry)(ctx);
    }

    /**
     * @brief Processes an event and executes a transition if one is defined and valid.
     *
     * The process order is:
     * 1. Find a matching transition (current state and event).
     * 2. Check the guard condition (if any).
     * 3. If NOT an internal transition: Call `on_exit` for the current state.
     * 4. Call the transition's `action` (if any).
     * 5. If NOT an internal transition: Set the new state and call `on_entry` for the new state.
     *
     * @param ev The event to process.
     * @param ctx The context object passed to all guards, actions, and callbacks.
     * @return true if a transition was successfully executed, false otherwise.
     */
    bool process(EventT ev, ContextT& ctx) {
        if (!current_.has_value()) return false;
        StateT cur = *current_;
        for (auto &t : transitions_) {
            if (t.from == cur && t.event == ev) {
                // 1. Check guard condition (if any)
                if (t.guard && !(*t.guard)(ctx)) continue;

                // 2. Execute exit callback if external transition
                if (!t.internal) {
                    if (auto *exit_cb = find_callbacks(cur); exit_cb && exit_cb->on_exit)
                        (*exit_cb->on_exit)(ctx);
                }

                // 3. Execute transition action (if any)
                if (t.action) (*t.action)(ctx);

                // 4. Update state and execute entry callback if external transition
                if (!t.internal) {
                    current_.emplace(t.to);
                    if (auto *entry_cb = find_callbacks(t.to); entry_cb && entry_cb->on_entry)
                        (*entry_cb->on_entry)(ctx);
                }
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Gets the current state of the state machine.
     * @return An `etl::optional` containing the current state, or empty if the machine hasn't started.
     */
    etl::optional<StateT> current_state() const { return current_; }

private:
    etl::vector<Transition, MaxTransitions> transitions_; ///< The collection of all possible transitions.
    etl::vector<StateCallbacks, MaxStates> state_callbacks_; ///< The collection of state entry/exit callbacks.
    etl::optional<StateT> current_; ///< The current state of the machine.

    /**
     * @brief Finds the StateCallbacks for a given state, or creates a new entry if it doesn't exist.
     * @param s The state to look for.
     * @return A pointer to the StateCallbacks structure for the state.
     */
    StateCallbacks* get_or_create_callbacks(StateT s) {
        for (auto &c : state_callbacks_) if (c.state == s) return &c;
        state_callbacks_.push_back({s, nullptr, nullptr});
        return &state_callbacks_.back();
    }

    /**
     * @brief Finds the StateCallbacks for a given state.
     * @param s The state to look for.
     * @return A pointer to the StateCallbacks structure for the state, or `nullptr` if not found.
     */
    StateCallbacks* find_callbacks(StateT s) {
        for (auto &c : state_callbacks_) if (c.state == s) return &c;
        return nullptr;
    }
};

} // namespace sm