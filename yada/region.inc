TM_LOG_OP(REG_GROW, TMgrowRegion, &ffi_type_pointer, pppp, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(REG_TRIANGULATE, TMretriangulate, &ffi_type_slong, pppp, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(REG_REFINE, TMregion_refine, &ffi_type_slong, ppp, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(REG_TRANSFERBAD, TMregion_transferBad, &ffi_type_void, pp, NULL, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);