TM_LOG_OP(HEAP_INSERT, TMheap_insert, &ffi_type_slong, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMHEAP_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_REPLAY);
TM_LOG_OP(HEAP_SIFTUP, TMsiftUp, &ffi_type_void, pl, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMHEAP_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_REPLAY);