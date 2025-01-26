
export render,
    RenderContextID,
    RenderConfig,
    
    Color,
    WHITE, BLACK, BLANK, MAGENTA, LIGHTGRAY, GRAY, DARKGRAY, YELLOW, GOLD, ORANGE, PINK, RED, MAROON,
    GREEN, LIME, DARKGREEN, SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
    
    init_renderer,
    renderer_should_close,
    begin_3d_rendering,
    end_3d_rendering,
    close_renderer,
    
    render_grid_floor,
    render_sphere,
    render_path

# All objects can have a default render function
function render(obj::Nothing)::Nothing end

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

# Colors stolen from raylib :)
const WHITE      = Color(r=255, g=255, b=255, a=255)   # White
const BLACK      = Color(r=0, g=0, b=0, a=255)         # Black
const BLANK      = Color(r=0, g=0, b=0, a=0)           # Blank (Transparent)
const MAGENTA    = Color(r=255, g=0, b=255, a=255)     # Magenta
const LIGHTGRAY  = Color(r=200, g=200, b=200, a=255)   # Light Gray
const GRAY       = Color(r=130, g=130, b=130, a=255)   # Gray
const DARKGRAY   = Color(r=80, g=80, b=80, a=255)      # Dark Gray
const YELLOW     = Color(r=253, g=249, b=0, a=255)     # Yellow
const GOLD       = Color(r=255, g=203, b=0, a=255)     # Gold
const ORANGE     = Color(r=255, g=161, b=0, a=255)     # Orange
const PINK       = Color(r=255, g=109, b=194, a=255)   # Pink
const RED        = Color(r=230, g=41, b=55, a=255)     # Red
const MAROON     = Color(r=190, g=33, b=55, a=255)     # Maroon
const GREEN      = Color(r=0, g=228, b=48, a=255)      # Green
const LIME       = Color(r=0, g=158, b=47, a=255)      # Lime
const DARKGREEN  = Color(r=0, g=117, b=44, a=255)      # Dark Green
const SKYBLUE    = Color(r=102, g=191, b=255, a=255)   # Sky Blue
const BLUE       = Color(r=0, g=121, b=241, a=255)     # Blue
const DARKBLUE   = Color(r=0, g=82, b=172, a=255)      # Dark Blue
const PURPLE     = Color(r=200, g=122, b=255, a=255)   # Purple
const VIOLET     = Color(r=135, g=60, b=190, a=255)    # Violet
const DARKPURPLE = Color(r=112, g=31, b=126, a=255)    # Dark Purple
const BEIGE      = Color(r=211, g=176, b=131, a=255)   # Beige
const BROWN      = Color(r=127, g=106, b=79, a=255)    # Brown
const DARKBROWN  = Color(r=76, g=63, b=47, a=255)      # Dark Brown

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

# renders a basic grid pattern on the x-y-plane centered at 0
function render_grid_floor(ctxid::RenderContextID, n_fields::Int, field_spacing::Float64)::Nothing
    return @ccall renderlib.render_grid_floor(ctxid::RenderContextID, n_fields::Cint, field_spacing::Cfloat)::Cvoid
end

function render_sphere(ctxid::RenderContextID, pos::Vec3f, radius::Float64, color::Color)::Nothing
    return @ccall renderlib.render_sphere(ctxid::RenderContextID, pos::Vec3f_32, radius::Cfloat, color::Color)::Cvoid
end

# renders an array of points as a contiguous path
function render_path(ctxid::RenderContextID, points::Vector{Vec3f_32}, length::Int, color::Color)::Nothing
    return @ccall renderlib.render_path(ctxid::RenderContextID, points::Ref{Vec3f_32}, length::Cint, color::Color)::Cvoid
end

### Default Render Functions ###

render(ctxid::RenderContextID, vec3f::Vec3f)::Nothing = render_sphere(ctxid, vec3f, 0.01, BLACK)

