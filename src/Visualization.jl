
export render,
    RenderContextID,
    Color,
    BLACK,
    WHITE,
    RenderConfig,
    init_renderer,
    renderer_should_close,
    begin_3d_rendering,
    end_3d_rendering,
    close_renderer,
    render_grid_floor,
    Sphere
    

# include("VisualizationTypes.jl")

### Properties ###

# function is_renderable(::ObjT) where {ObjT <: SimObject} return false end

### Rendering ###

# function get_renderables(obj::ObjT)::Vector{Renderable} where {ObjT <: SimObject} end
function render()::Nothing end

# function render_error(msg::Cstring) # do this later, shold be passed as func ptr to the C++ renderer, which calls it with the error string

const renderlib = "TinyDronesRender/bin/libtinydronesrender.so"

struct RenderContextID
    id::Int64
end

@kwdef struct Color
    r::UInt8 = 0
    g::UInt8 = 0
    b::UInt8 = 0
    a::UInt8 = 255
end

const BLACK = Color(r=0, g=0, b=0, a=255)
const WHITE = Color(r=255, g=255, b=255, a=255)

@kwdef struct RenderConfig
    window_width::Int32 = 800
    window_height::Int32 = 600
    FPS::Int32 = 60
    background_color::Color = WHITE
end

function init_renderer(render_config::RenderConfig)::RenderContextID
    return @ccall renderlib.init_renderer(render_config::RenderConfig)::RenderContextID
end

function renderer_should_close(ctxid::RenderContextID)::Bool
    return @ccall renderlib.renderer_should_close(ctxid::RenderContextID)::Cuchar
end

function begin_3d_rendering(ctxid::RenderContextID)::Nothing
    return @ccall renderlib.begin_3d_rendering(ctxid::RenderContextID)::Cvoid
end

function end_3d_rendering(ctxid::RenderContextID)::Nothing
    return @ccall renderlib.end_3d_rendering(ctxid::RenderContextID)::Cvoid
end

function close_renderer(ctxid::RenderContextID)::Nothing
    return @ccall renderlib.close_renderer(ctxid::RenderContextID)::Cvoid
end

### Render Functions ###

function render_grid_floor(ctxid::RenderContextID, n_fields::Int32, field_spacing::Float64)::Nothing
    return @ccall renderlib.render_grid_floor(ctxid::RenderContextID, n_fields::Cint, field_spacing::Cdouble)::Cvoid
end

@kwdef mutable struct Sphere
    x::Vec3f = Vec3f(0, 0, 0)
    color::Color = BLACK
    radius::Float64 = 0.01
end

function render(ctxid::RenderContextID, sphere::Sphere)::Nothing
    return @ccall renderlib.render_sphere(ctxid::RenderContextID, sphere::Sphere)::Cvoid
end

render(ctxid::RenderContextID, vec3f::Vec3f)::Nothing = render(ctxid, Sphere(x=vec3f))
