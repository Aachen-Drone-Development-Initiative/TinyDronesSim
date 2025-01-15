

# function get_renderables(obj::ObjT)::Vector{Any} where {ObjT <: SimObject}
#     fields = fieldnames(type)
#     field_types = fieldtypes(type)
#     @assert(length(fields) == length(field_types))
#     n_fields = length(fields)
#     renderables = Any[]
#     for i in 1:n_fields
#         if hasmethod(get_renderables, (field_types[i],))
#             push!(renderables, get_renderables(getfield(obj, fieldsp[i])))
#         else if is_renderable(field_types[i]())
#             push!(renderables, getfield(obj, fields[i]))
#         end
#     end
#     return renderables
# end

# @generated function render(obj::ObjT)::Nothing where {ObjT <: SimObject}
#     if hasmethod(get_renderables, (ObjT,))
#         renderables = get_renderables(obj)
#         println(renderables)
#     else
#         @error("The object $obj of type $ObjT has no renderables.")
#     end
        
# end

