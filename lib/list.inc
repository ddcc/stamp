TM_LOG_OP(LIST_PREVIOUS, TMfindPrevious, &ffi_type_pointer, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_REPLAY);
TM_LOG_OP(LIST_IT_HASNEXT, NULL, &ffi_type_slong, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_IT_NEXT, NULL, &ffi_type_pointer, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_ALLOC, TMlist_alloc, &ffi_type_pointer, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_FREE, TMlist_free, &ffi_type_void, p, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_GETSZ, TMlist_getSize, &ffi_type_slong, p, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_ISEMPTY, TMlist_isEmpty, &ffi_type_slong, p, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_FIND, TMlist_find, &ffi_type_pointer, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_INSERT, TMlist_insert, &ffi_type_slong, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(LIST_REMOVE, TMlist_remove, &ffi_type_slong, pp, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMLIST_MERGE : NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);