TM_LOG_OP(RSV_MAKE, reservation_make, &ffi_type_slong, p, TMreservation_merge, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(RSV_CANCEL, reservation_cancel, &ffi_type_slong, p, TMreservation_merge, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
TM_LOG_OP(RSV_CHECK, checkReservation, &ffi_type_void, p, TMreservation_merge, STM_MERGE_POLICY_FUNCTION, STM_MERGE_POLICY_FUNCTION);
