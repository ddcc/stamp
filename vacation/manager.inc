TM_LOG_OP(MGR_ADDRESERVE, addReservation, &ffi_type_slong, plll, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_ADDCUSTOMER, manager_addCustomer, &ffi_type_slong, pl, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_DELCUSTOMER, manager_deleteCustomer, &ffi_type_slong, pl, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_DELFLIGHT, manager_deleteFlight, &ffi_type_slong, pl, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_QUERYFREE, queryNumFree, &ffi_type_slong, pl, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMMANAGER_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_QUERYPRICE, queryPrice, &ffi_type_slong, pl, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMMANAGER_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_RESERVE, reserve, &ffi_type_slong, pplli, ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? TMMANAGER_MERGE : NULL, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(MGR_CANCEL, cancel, &ffi_type_slong, pplli, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
