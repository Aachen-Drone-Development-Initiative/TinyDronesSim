# This file contains anything which helps with metaprogramming,
# most notably a set of functions for finding fields within a composite type (struct)
# which have certain propeties (can be used as arguments for certain function calls,
# for example get_mass(), which means the helper function finds all fields 'which have a mass')

export @error_nonsensecall,
    @has,
    FunctionCall,
    find_fields_for_method,
    find_fields_for_methods,
    find_fields_for_method_multiargs,
    find_fields_for_methods_multiargs,
    find_fields_for_methods_multiargs_impl,
    find_fields_for_method_v,
    find_fields_for_methods_v,
    find_fields_for_method_multiargs_v,
    find_fields_for_methods_multiargs_v,
    find_fields_for_methods_multiargs_v_impl,
    field_set_to_field_expr,
    find_fields_for_methods_v_t
    

# error macro for generic functions for when they need to deny their provided arguments
macro error_nonsensecall(obj, name)
    return :(@error("calling $name on '$obj' of type $(typeof(obj)) doesn't make any sense."))
end

macro error_nonsensecall(obj, name, msg)
    return :(@error("calling $name on '$obj' of type $(typeof(obj)) doesn't make any sense.\n $msg."))
end


tuplejoin(x) = x
tuplejoin(x, y) = (x..., y...)
tuplejoin(x, y, z...) = (x..., tuplejoin(y, z...)...)

# simplification for hasmethod(...)
macro has(method, type)
    return :(hasmethod($(esc(method)), ($(esc(type)),)))
end

macro has(method, type, right_args_tuple)
    return :(hasmethod($(esc(method)), tuplejoin(($(esc(type)),), $(esc(right_args_tuple)))))
end

macro has(method, left_args_tuple, type, right_args_tuple)
    return :(hasmethod($(esc(method)), tuplejoin($(esc(left_args_tuple)), ($(esc(type)),), $(esc(right_args_tuple)))))
end


struct FunctionCall
    method::Function
    left_args::Tuple
    right_args::Tuple
end

#=
Returns a vector of all fields (names of objects inside the type) which can be passed as argument to a method.
If a field can't be passed as argument to the method, then it's fields will be checked recursively.
=#
find_fields_for_method(type::Type, method::Function) = find_fields_for_methods_multiargs_impl(type, [method], false)
find_fields_for_methods(type::Type, methods::Vector{Function}) = find_fields_for_methods_multiargs_impl(type, methods, false)
find_fields_for_method_multiargs(type::Type, function_call::FunctionCall) = find_fields_for_methods_multiargs_impl(type, [function_call], true)
find_fields_for_methods_multiargs(type::Type, function_calls::Vector{FunctionCall}) = find_fields_for_methods_multiargs_impl(type, function_calls, true)

function field_set_to_field_expr(field_set::Vector{Symbol})
    field_expr::Union{Expr, Symbol} = field_set[1]
    for i in 2:length(field_set)
        field_expr = :($field_expr.$(field_set[i]))
    end
    return field_expr
end

function field_set_to_field_expr(field_set::Vector{Tuple{Symbol, DataType}})
    field_expr::Union{Expr, Symbol} = field_set[1][1]
    for i in 2:length(field_set)
        field_expr = :($field_expr.$(field_set[i][1]))
    end
    return field_expr
end

function find_fields_for_methods_multiargs_impl(type::Type, function_calls::Vector, has_multiargs::Bool)::Vector{Union{Expr, Symbol}}
    field_sets = find_fields_for_methods_multiargs_v_impl(type, function_calls, has_multiargs)
    field_exprs = Vector{Union{Expr, Symbol}}()
    for field_set in field_sets
        push!(field_exprs, field_set_to_field_expr(field_set))
    end
    return field_exprs
end

#=
Just as the above functions, but instead of returning a vector expressions like 'a.b.c' it returns
a vector of vectors containing [a,b,c]
=#
find_fields_for_method_v(type::Type, method::Function) = find_fields_for_methods_multiargs_v_impl(type, [method], false)
find_fields_for_methods_v(type::Type, methods::Vector{Function}) = find_fields_for_methods_multiargs_v_impl(type, methods, false)
find_fields_for_method_multiargs_v(type::Type, function_call::FunctionCall) = find_fields_for_methods_multiargs_v_impl(type, [function_call], true)
find_fields_for_methods_multiargs_v(type::Type, function_calls::Vector{FunctionCall}) = find_fields_for_methods_multiargs_v_impl(type, function_calls, true)

function find_fields_for_methods_multiargs_v_impl(type::Type, function_calls::Vector, has_multiargs::Bool)::Vector{Vector{Symbol}}
    typed_field_sets = find_fields_for_methods_multiargs_v_t_impl(type, function_calls, has_multiargs)
    field_sets = Vector{Vector{Symbol}}()
    for typed_field_set in typed_field_sets
        push!(field_sets, Vector{Symbol}())
        for typed_field in typed_field_set
            push!(field_sets[end], typed_field[1])
        end
    end
    return field_sets
end

#=
Just as the above functions, but instead of returning a vector of vectors containing [a,b,c]
it returns [(a, typeof(a)), (b, typeof(b)), (c, typeof(c))]
=#
find_fields_for_methods_v_t(type::Type, methods::Vector{Function}) = find_fields_for_methods_multiargs_v_t_impl(type, methods, false)

function find_fields_for_methods_multiargs_v_t_impl(type::Type, function_calls::Vector, has_multiargs::Bool)::Vector{Vector{Tuple{Symbol, DataType}}}
    typed_field_sets = Vector{Vector{Tuple{Symbol, DataType}}}()
    try
        fields = fieldnames(type)
        field_types = fieldtypes(type)
        @assert(length(fields) == length(field_types))
        n_fields = length(fields)
        if n_fields != 0
            for i in 1:n_fields
                has_methods = true
                for function_call in function_calls
                    if has_multiargs
                        has_methods = has_methods && hasmethod(function_call.method, tuplejoin(function_call.left_args, (field_types[i],), function_call.right_args))
                    else
                        has_methods = has_methods && hasmethod(function_call, (field_types[i],))
                    end
                end
                if has_methods
                    push!(typed_field_sets, [(Symbol(fields[i]), field_types[i])])
                else
                    sub_typed_field_sets = find_fields_for_methods_multiargs_v_t_impl(field_types[i], function_calls, has_multiargs)
                    for sub_field_set in sub_typed_field_sets
                        insert!(sub_field_set, 1, (Symbol(fields[i]), field_types[i]))
                        push!(typed_field_sets, sub_field_set)
                    end
                end
            end
        end
    catch end
    return typed_field_sets
end
