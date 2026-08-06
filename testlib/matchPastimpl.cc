

// template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
// requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && 
//           mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
// constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple& cases) {
//     constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;

//     using CoreReturnType  = decltype(execute_action(default_action, ctx));
//     using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;

//     // Control flow result structure
//     struct CaseResult {
//         bool executed = false;
//         bool jump_taken = false;
//         used_std::size_t next_index = 0;
//         std::optional<CleanReturnType> value;
//     };

//     // Helper to evaluate a single case at static index 'I'
//     auto dispatch_single_case = [&]<used_std::size_t I>() -> CaseResult {
//         CaseResult res;
//         auto& current_case = used_std::get<I>(cases);
//         using RawCaseType   = used_std::remove_cvref_t<decltype(current_case)>;

//         if (apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key))) {
//             using ActionResultType = decltype(execute_action(current_case.action, ctx));

//             // Handle VOID-returning actions
//             if constexpr (used_std::is_same_v<ActionResultType, void>) {
//                 execute_action(current_case.action, ctx); // Execute side effects
//                 res.executed = true;                       // Void actions are terminal
//             } 
//             // Handle NON-VOID-returning actions (signals, values, etc.)
//             else {
//                 decltype(auto) action_result = execute_action(current_case.action, ctx);
//                 using CaseActionDecay = used_std::decay_t<decltype(action_result)>;

//                 // 1. Handle Goto Jump Signal
//                 if constexpr (IsPureGoto<CaseActionDecay> || IsValueGoto<CaseActionDecay>) {
//                     res.next_index = find_target_label_index_v<CaseActionDecay::label, used_std::remove_cvref_t<CasesTuple>>;
//                     res.jump_taken = true;
//                 } 
//                 // 2. Handle Fallthrough Signal
//                 else if constexpr (mini_concepts::IsPureFallthrough<CaseActionDecay> || mini_concepts::IsValueFallthrough<CaseActionDecay>) {
//                     res.next_index = I + 1;
//                     res.jump_taken = true;
//                 }
//                 // 3. Regular Terminal Execution returning a Value
//                 else {
//                     if constexpr (!used_std::is_same_v<CleanReturnType, Wildcard>) {
//                         res.value = std::move(action_result);
//                     }
//                     res.executed = true;
//                 }
//             }
//         }
//         return res;
//     };

//     // Generate static jump table mapping runtime index to static handler function
//     using HandlerFn = CaseResult(*)(decltype(dispatch_single_case)&);
    
//     constexpr auto dispatch_table = []<used_std::size_t... Is>(used_std::index_sequence<Is...>) {
//         return std::array<HandlerFn, sizeof...(Is)>{
//             [](decltype(dispatch_single_case)& runner) { return runner.template operator()<Is>(); }...
//         };
//     }(used_std::make_index_sequence<TotalCases>{});

//     // Execute state machine driver
//     used_std::size_t active_index = 0;
//     while (active_index < TotalCases) {
//         CaseResult step = dispatch_table[active_index](dispatch_single_case);

//         if (step.executed) {
//             if constexpr (!used_std::is_same_v<CleanReturnType, Wildcard>) {
//                 return *step.value;
//             } else {
//                 return;
//             }
//         }

//         if (step.jump_taken) {
//             active_index = step.next_index;
//         } else {
//             active_index++;
//         }
//     }

//     // Default Fallback
//     if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//         execute_action(default_action, ctx);
//     } else {
//         return execute_action(default_action, ctx);
//     }
// }
// template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
// requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && 
//           mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
// constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
//     constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;

//     using CoreReturnType  = decltype(execute_action(default_action, ctx));
//     using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;
//     used_std::size_t active_index = 0;

//     // Iterative loop handles all jumps, fallthroughs, and sequential progression
//     while (active_index < TotalCases) {
//         bool jump_taken = false;
//         bool executed   = false;

//         std::tuple<CleanReturnType,Wildcard> result;

//         // Dispatch logic for the current active_index
//         [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> std::tuple<CleanReturnType,Wildcard> {
//             ((active_index == Is ? (void)[&]() {
//                 auto&& current_case = used_std::get<Is>(cases);
//                 using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;

//                 // Check key match
//                 if (apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key))) {
//                     using RawActionResult = decltype(execute_action(current_case.action, ctx));
//                     using CaseActionDecay = used_std::decay_t<RawActionResult>;

//                     // 1. Handle Goto Jump
//                     if constexpr (IsPureGoto<CaseActionDecay> || IsValueGoto<CaseActionDecay>) {
//                         constexpr auto TargetIndex = find_target_label_index_v<CaseActionDecay::label, used_std::remove_cvref_t<CasesTuple>>;
//                         active_index = TargetIndex; // Update target for next iteration
//                         jump_taken = true;
//                     } 
//                     // 2. Handle Fallthrough
//                     else if constexpr (mini_concepts::IsPureFallthrough<CaseActionDecay> || mini_concepts::IsValueFallthrough<CaseActionDecay>) {
//                         active_index = Is + 1; // Fall through to next case
//                         jump_taken = true;
//                     }
//                     // 3. Regular Terminal Execution
//                     else {
//                         if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//                             execute_action(current_case.action, ctx);
//                         } else {
//                             used_std::get<0>(result) = execute_action(current_case.action, ctx);
//                         }
//                         executed = true;
//                     }
//                 }
//                 return execute_action(current_case.action, ctx);
//             }() : (void)0), ...);
//             return {};
//         }(used_std::make_index_sequence<TotalCases>{});

//         // Terminal case executed -> Exit loop and return
//         if (executed) {
//             if constexpr (!used_std::is_same_v<CleanReturnType, Wildcard>) {
//                 return used_std::get<0>(result);
//             } else {
//                 return;
//             }
//         }

//         // If no match occurred and no goto jump was triggered, proceed sequentially
//         if (!jump_taken) {
//             active_index++;
//         }
//     }

//     // Default Action (if no match or out-of-bounds jump)
//     if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//         execute_action(default_action, ctx);
//     } else {
//         return execute_action(default_action, ctx);
//     }
// }
// template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
// requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && 
//           mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
// constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
//     constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;

//     using CoreReturnType  = decltype(execute_action(default_action, ctx));
//     using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;

//     auto unrolled_matrix_router = []<used_std::size_t... Is>(
//         const TargetType& target, 
//         DefaultType&& default_action, 
//         ContextTuple& ctx, 
//         CasesTuple&& cases,
//         used_std::index_sequence<Is...>
//     ) -> CleanReturnType {
        
//         // Helper lambda to sequentially iterate without triggering all lambdas or default actions repeatedly
//         auto evaluate_case = [&]<used_std::size_t I>() -> CleanReturnType {
//             auto&& current_case = used_std::get<I>(cases);

//             using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
//             using RawActionResult   = decltype(execute_action(current_case.action, ctx));
//             using CaseActionDecay   = used_std::decay_t<RawActionResult>;
            
//             if (apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key))) {

//                 // Handle Fallthrough Cases
//                 if constexpr (mini_concepts::IsPureFallthrough<CaseActionDecay> || mini_concepts::IsValueFallthrough<CaseActionDecay>) {
//                     if constexpr (I + 1 < TotalCases) { // FIX 1: Prevent out-of-bounds access
//                         auto&& next_case = used_std::get<I + 1>(cases);
//                         using RawNextCase = used_std::remove_cvref_t<decltype(next_case)>;
                        
//                         if (evaluate_match(target, next_case.key)) {
//                             if constexpr (used_std::is_same_v<RawNextCase, Wildcard>) {
//                                 execute_action(next_case.action, ctx);
//                                  // Signal execution handled
//                             } else {
//                                 return execute_action(next_case.action, ctx);
//                             }
//                         }
//                     }
//                 }
//                 // Handle Pure Goto
//                 else if constexpr (IsPureGoto<CaseActionDecay>) {
//                     constexpr auto labelIndex = find_target_label_index_v<CaseActionDecay::label, used_std::remove_cvref_t<CasesTuple>>;
                    
//                     auto&& jump_case = used_std::get<labelIndex>(cases);
//                     if (evaluate_match(target, jump_case.key)) {
//                         if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//                             execute_action(jump_case.action, ctx); // FIX 4: Call jump_case.action, not current_case.action
//                             return Wildcard{};
//                         } else {
//                             return execute_action(jump_case.action, ctx);
//                         }
//                     }
//                 }
//                 // Handle Value Goto
//                 else if constexpr (IsPureGoto<CaseActionDecay>) {
//                     constexpr auto labelIndex = find_target_label_index_v<CaseActionDecay::label, used_std::remove_cvref_t<CasesTuple>>;
                    
//                     auto&& jump_case = used_std::get<labelIndex>(cases);
//                     if (evaluate_match(target, jump_case.key)) {
//                         if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//                             execute_action(jump_case.action, ctx);
//                             return Wildcard{};
//                         } else {
//                             return execute_action(jump_case.action, ctx);
//                         }
//                     }
//                 }
//                 // Regular Match Case
//                 else {
//                     if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//                         execute_action(current_case.action, ctx);
//                         return Wildcard{};
//                     } else {
//                         return execute_action(current_case.action, ctx);
//                     }
//                 }
//             }
//             return {}; // No match on this case
//         };
        
//         if constexpr (used_std::is_same_v<CleanReturnType, Wildcard>) {
//             bool executed = false;
//             // Short-circuit iteration as soon as a case matches
//             ((!executed && (
//                 [](const TargetType& target,CasesTuple&& cases,bool* cnd,auto* ev) {
//                     auto&& current_case = used_std::get<Is>(cases);
//                     using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
//                     if (apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key))) {
//                         ev->template operator()<Is>();
//                         *cnd = true;
//                     }
//                 }(target,used_std::forward<CasesTuple>(cases),&executed,&evaluate_case), 0
//             )), ...);

//             if (!executed) {
//                 execute_action(default_action, ctx);
//             }
//         } else {
//             bool executed = false;
//             CleanReturnType result{};
            
//             // FIX 3: Short-circuiting expansion for returning non-void types
//             ((!executed && (
//                 [](const TargetType& target,CasesTuple&& cases,CleanReturnType* ret,bool* cnd,auto* ev) {
//                     auto&& current_case = used_std::get<Is>(cases);
//                     using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
//                     if (apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key))) {
//                         *ret = ev->template operator()<Is>();
//                         *cnd = true;
//                     }
//                 }(target,used_std::forward<CasesTuple>(cases),&result,&executed,&evaluate_case), 0
//             )), ...);

//             if (executed) {
//                 return result;
//             }
//             return execute_action(default_action, ctx);
//         }
//         return {};
//     };

//     return unrolled_matrix_router(
//         target, 
//         used_std::forward<DefaultType>(default_action), 
//         ctx, 
//         used_std::forward<CasesTuple>(cases),
//         used_std::make_index_sequence<TotalCases>{}
//     );
// }

// static_assert(used_std::is_same_v<testindex, used_std::index_sequence<1>>,"" );

// Unrolled Matrix State Router Engine Loop
// template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
// requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
// constexpr auto universal_switch_matrix2(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
//     constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;
    
//     using CoreReturnType  = decltype(execute_action(default_action, ctx));
//     using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;
//     // Updated: Pass target to resolve correct return type
    
//     auto unrolled_matrix_router = []<used_std::size_t... Is>(const TargetType& target, DefaultType&& default_action, 
//         ContextTuple& ctx, CasesTuple&& cases,used_std::index_sequence<Is...>) -> CleanReturnType {
//         using RawFirstCase = used_std::remove_cvref_t<decltype(used_std::get<0>(cases))>;
//         using LabelType    = used_std::remove_cvref_t<decltype(RawFirstCase::label)>;
        
//         LabelType target_jump_label{};
//         bool matched = false;
//         bool force_execute_next = false;
//         bool jump_requested = false;
        
//         used_std::size_t loop_guard = 0;
//         constexpr used_std::size_t MaxAllowedJumps = TotalCases * 2;

//         // Separate void and non-void return type pathways
//         if constexpr (used_std::is_same_v<CleanReturnType, void>) {
//             while (loop_guard++ < MaxAllowedJumps) {
//                 bool current_iteration_jumped = false;
                
//                 ([&]() {
//                     if (matched && !force_execute_next && !jump_requested) return;
                    
//                     auto&& current_case = used_std::get<Is>(cases);
//                     using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
                    
//                     bool should_execute = false;
                    
//                     if (jump_requested) {
//                         if (current_case.label == target_jump_label) {
//                             should_execute = true;
//                         }
//                     } else if (force_execute_next) {
//                         should_execute = true;
//                     } else if (!matched) {
//                         should_execute = apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key));
//                     }

//                     if (should_execute) {
//                         matched = true;
//                         force_execute_next = false;
//                         jump_requested = false;
                        
//                         execute_action(current_case.action, ctx);
//                         using ActionDecay = used_std::decay_t<CleanReturnType>;

//                         if constexpr (mini_concepts::IsPureFallthrough<ActionDecay> || mini_concepts::IsValueFallthrough<ActionDecay>) {
//                             force_execute_next = true;
//                         }
//                         else if constexpr (IsPureGoto<ActionDecay>) {
//                             target_jump_label = is_goto_case<ActionDecay>::label;
//                             constexpr auto StaticLabel = is_goto_value<ActionDecay>::label;
//                             auto labelIndex = find_target_label_index_t<StaticLabel, CasesTuple>{};
//                             std::cout << used_std::get<labelIndex>(cases).label.data << "\n";
//                             jump_requested = true;
//                             current_iteration_jumped = true;
//                         }
//                         else if constexpr (IsValueGoto<ActionDecay>) {
//                             target_jump_label = is_goto_value<ActionDecay>::label;
//                             auto labelIndex = find_target_label_index_t<target_jump_label.data, CasesTuple>{};
//                             []<size_t... Iss> (std::index_sequence<Iss...>){
//                                 ((std::cout << Iss << " "),...);
//                             }(labelIndex);
//                             jump_requested = true;
//                             current_iteration_jumped = true;
//                         }
//                     }
//                 }(), ...);
//                 if (!current_iteration_jumped && !force_execute_next) {
//                     break;
//                 }
//             }
            
//             if (!matched || force_execute_next || jump_requested) {
//                 // Updated: Passed target to default execute_action
//                 execute_action(default_action, ctx);
//             }
//         } 
//         else {
//             CleanReturnType result{};

//             while (loop_guard++ < MaxAllowedJumps) {
//                 bool current_iteration_jumped = false;
                
//                 ([&]() {
//                     if (matched && !force_execute_next && !jump_requested) return;

//                     auto&& current_case = used_std::get<Is>(cases);
//                     using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
                    
//                     bool should_execute = false;
                    
//                     if (jump_requested) {
//                         if (current_case.label == target_jump_label) {
//                             should_execute = true;
//                         }
//                     } else if (force_execute_next) {
//                         should_execute = true;
//                     } else if (!matched) {
//                         should_execute = apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key));
//                     }

//                     if (should_execute) {
//                         matched = true;
//                         force_execute_next = false;
//                         jump_requested = false;
                        
//                         // Updated: Passed target to execute_action
//                         decltype(auto) action_res = execute_action(current_case.action, ctx);
//                         using ActionDecay = used_std::decay_t<decltype(action_res)>;

//                         if constexpr (mini_concepts::IsPureFallthrough<ActionDecay>) {
//                             force_execute_next = true;
//                         }
//                         else if constexpr (mini_concepts::IsValueFallthrough<ActionDecay>) {
//                             result = action_res.value;
//                             force_execute_next = true;
//                         }
//                         else if constexpr (IsPureGoto<ActionDecay>) {
//                             target_jump_label = is_goto_case<ActionDecay>::label;
                            
//                             // constexpr auto StaticLabel = is_goto_case<ActionDecay>::label;
//                             auto labelIndex = find_target_label_index_t<ActionDecay::label, used_std::remove_cvref_t<CasesTuple>>{};
//                             static_assert(used_std::is_same_v<decltype(labelIndex), used_std::index_sequence<3>>,"");
//                             []<size_t... Iss> (std::index_sequence<Iss...>){
//                                 ((std::cout << Iss << " \n"), ...);
//                             }(labelIndex);
//                             // std::cout << used_std::get<labelIndex>(cases).label.data << "\n";
//                             jump_requested = true;
//                             current_iteration_jumped = true;
//                         }
//                         else if constexpr (IsValueGoto<ActionDecay>) {
//                             result = action_res.value;
//                             target_jump_label = is_goto_value<ActionDecay>::label;
//                             jump_requested = true;
//                             current_iteration_jumped = true;
//                         }
//                         else {
//                             result = action_res;
//                         }
//                     }
//                 }(), ...);

//                 if (!current_iteration_jumped && !force_execute_next) {
//                     break;
//                 }
//             }
            
//             if (matched && !force_execute_next && !jump_requested) {
//                 return result;
//             }
            
//             // Updated: Passed target to default execute_action
//             return execute_action(default_action, ctx);
//         }
//     };

//     return unrolled_matrix_router(target, used_std::forward<DefaultType>(default_action), ctx, used_std::forward<CasesTuple>(cases),used_std::make_index_sequence<TotalCases>{});
// }
