TM_LOG_OP(LRN_FINDINSERT, TMfindBestInsertTask, &learner_ffi, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LRN_FINDREMOVE, TMfindBestRemoveTask, &learner_ffi, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LRN_FINDREVERSE, TMfindBestReverseTask, &learner_ffi, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LRN_POPTASK, TMpopTask, &ffi_type_pointer, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
