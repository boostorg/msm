    boost::mp11::mp_list<
        boost::mp11::mp_list<
            (anonymous namespace)::my_machine_::State1,
            boost::mp11::mp_list<
                boost::msm::back::state_machine<(anonymous namespace)::
                                                    my_machine_,
                                                boost::parameter::void_,
                                                boost::parameter::void_,
                                                boost::parameter::void_,
                                                boost::parameter::void_>::
                    _row_<boost::msm::front::state_machine_def<
                        (anonymous namespace)::my_machine_,
                        boost::msm::front::default_base_state>::
                              _row<(anonymous namespace)::my_machine_::State1,
                                   boost::msm::front::none,
                                   (anonymous namespace)::my_machine_::
                                       State2>>>>,
        boost::mp11::mp_list<
            (anonymous namespace)::my_machine_::State2,
            boost::mp11::mp_list<
                boost::msm::back::state_machine<(anonymous namespace)::
                                                    my_machine_,
                                                boost::parameter::void_,
                                                boost::parameter::void_,
                                                boost::parameter::void_,
                                                boost::parameter::void_>::
                    a_row_<boost::msm::front::state_machine_def<
                        (anonymous namespace)::my_machine_,
                        boost::msm::front::default_base_state>::
                               a_row<(anonymous namespace)::my_machine_::State2,
                                     boost::msm::front::none,
                                     (anonymous namespace)::my_machine_::State3,
                                     &(anonymous namespace)::my_machine_::
                                         State2ToState3>>>>,
        boost::mp11::mp_list<
            (anonymous namespace)::my_machine_::State3,
            boost::mp11::mp_list<
                boost::msm::back::state_machine<
                    (anonymous namespace)::my_machine_,
                    boost::parameter::void_,
                    boost::parameter::void_,
                    boost::parameter::void_,
                    boost::parameter::void_>::
                    row_<boost::msm::front::state_machine_def<
                        (anonymous namespace)::my_machine_,
                        boost::msm::front::default_base_state>::
                             row<(anonymous namespace)::my_machine_::State3,
                                 boost::msm::front::none,
                                 (anonymous namespace)::my_machine_::State4,
                                 &(anonymous namespace)::my_machine_::
                                     State3ToState4,
                                 &(anonymous namespace)::my_machine_::
                                     always_true>>,
                boost::msm::front::state_machine_def<
                    (anonymous namespace)::my_machine_,
                    boost::msm::front::default_base_state>::
                    g_row<(anonymous namespace)::my_machine_::State3,
                          boost::msm::front::none,
                          (anonymous namespace)::my_machine_::State4,
                          &(anonymous namespace)::my_machine_::always_false>>>>>