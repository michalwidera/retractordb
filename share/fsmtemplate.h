/*Copyright 2010-2012 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918
*/

/*
 *
 * This code is based on example from:
 * Jezyk C++. Metaprogramowanie za pomoca szablonow
 * Autorzy: David Abrahams, Aleksey Gurtovoy
 *
 */

#ifndef FSMTEMPLATE_H
#define FSMTEMPLATE_H

#include <boost/mpl/fold.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/assert.hpp>
#include <cassert>
#include <vector>

namespace fsm {

using namespace std ;
using namespace boost ;
using namespace boost::mpl::placeholders;

template< class Transition
        , class Next
        >
struct event_dispatcher
{
    typedef typename Transition::fsm_t fsm_t;
    typedef typename Transition::event event;

    static int dispatch(fsm_t& fsm, int state, event const& e)
    {
        if (state == Transition::current_state)
        {
            Transition::execute(fsm, e);
            return Transition::next_state;
        }
        else // przejscie do nastepnego wezla w lancuchu
        {
            return Next::dispatch(fsm, state, e);
        }
    }
};

template <class Derived> class state_machine;

struct default_event_dispatcher
{
    template<class FSM, class Event>
    static int dispatch(state_machine<FSM>& m, int state, Event const& e)
    {
        return m.call_no_transition(state, e);
    }
};


template<
        class Table,
        class Event
        >
struct generate_dispatcher;

template<class Derived>
class state_machine
{

protected:

    template< int CurrentState
            , class Event
            , int NextState
            , void (Derived::*action)(Event const&)
            >
    struct row
    {
        static int const current_state = CurrentState;
        static int const next_state = NextState;
        typedef Event event;
        typedef Derived fsm_t;

        static void execute(Derived& fsmParam, Event const& e)
        {
            (fsmParam.*action)(e);
        }
    };


    friend class default_event_dispatcher;

    template <class Event>
    int call_no_transition(int state, Event const& e)
    {
        return static_cast<Derived*>(this)->no_transition(state, e);
    }

    state_machine()
        : state(Derived::initial_state)
    {
    }

private:
    int state;

public:
    template <class Event>
    int no_transition(int state, Event const& e)
    {
        assert(false);
        return state;
    }

    template<class Event>
    int process_event(Event const& evt)
    {
        // wygenerowanie typu dyspozytora
        typedef typename generate_dispatcher<
                typename Derived::transition_table, Event
                >::type dispatcher;

        // rozdzielenie zdarzenia
        this->state = dispatcher::dispatch(
                    *static_cast<Derived*>(this)        // rzutowanie w dol z CRTP
                    , this->state
                    , evt
                    );

        // zwrocenie nowego stanu
        return this->state;
    }

};


// pobranie zdarzenia powiazanego z transakcja
template <class Transition>
struct transition_event
{
    typedef typename Transition::event type;
};

template<class Table, class Event>
struct generate_dispatcher
        : mpl::fold<
                   mpl::filter_view<   // wybranie wierszy wyzwalanych przez Event
                                   Table
                                   , boost::is_same<Event, transition_event<_1> >
                                   >
                   , default_event_dispatcher
                   , event_dispatcher<_2,_1>
                   >
{};


} //endof namespace fsm

#endif // FSMTEMPLATE_H
