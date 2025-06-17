
log_info(args) = println("INFO: ", args...)
log_warning(args) = println("WARNING: ", args...)
log_soft_error(args) = println("ERROR: ", args...)

function log_hard_error(args)
    println("FATAL ERROR: ", args...)
    exit(1)
end

function log_todo(args)
    println("HIT A TODO: ", args...)
    exit(0)
end
