#include <logging.hpp>
#include <object_manager.hpp>
#include <cstdint>
#include <joystick.hpp>
#include <math.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <linux/joystick.h>

Joystick_ID Joystick::create()
{
    Joystick* joystick = new Joystick {};
    return (joystick->m_id = g_objm.add_object(joystick));
}

TDS_API Joystick_ID joystick_create() { return Joystick::create(); }

Joystick_ID Joystick::create_and_connect(const char* device)
{
    Joystick* joystick = new Joystick {};
    joystick->connect(device);
    return (joystick->m_id = g_objm.add_object(joystick));
}

TDS_API Joystick_ID joystick_create_and_connect(const char* device)
{
    return Joystick::create_and_connect(device);
}

bool Joystick::connect(const char* device)
{
    if (device[0] == '\0') {
        device = "/dev/input/js0";
    }

    if (m_connected) {
        this->disconnect();
    }

    m_joystick_file = open(device, O_RDONLY | O_NONBLOCK);
    if (m_joystick_file == -1) {
        tds_soft_error("Failed to open the Joystick at '%s'", device);
        return false;
    }
    
    if (ioctl(m_joystick_file, JSIOCGAXES, &m_raw.axes_length) == -1) {
        tds_soft_error("Failed to get Joystick axes count");
        return false;
    }

    if (ioctl(m_joystick_file, JSIOCGBUTTONS, &m_raw.buttons_length) == -1) {
        tds_soft_error("Failed to get Joystick button count");
        return false;
    }
    
    m_connected = true;
    return true;
}

TDS_API bool joystick_connect(Joystick_ID joystick_id, const char* device)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;

    return joystick->connect(device);
}

Joystick::~Joystick() { disconnect(); }

void Joystick::disconnect()
{
    close(m_joystick_file);
    m_connected = false;
}

TDS_API bool joystick_disconnect(Joystick_ID joystick_id)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;

    joystick->disconnect();
    return true;
}

bool Joystick::is_connected() { return m_connected; }

TDS_API bool joystick_is_connected(Joystick_ID joystick_id)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;

    return joystick->is_connected();
}

bool Joystick::process_event()
{
    js_event event;
    size_t axis;

    if (!m_connected) {
        tds_soft_error("Joystick not connected");
        return false;
    }

    ssize_t bytes = read(m_joystick_file, &event, sizeof(event));
    while (bytes == sizeof(event)) {
        switch (event.type) {
        case JS_EVENT_BUTTON:
            if (event.number < MAX_JOYSTICK_LENGTH) {
                m_raw.buttons[event.number] = (bool) event.value;
            } else {
                tds_soft_error("exceeded MAX_JOYSTICK_LENGTH");
            }
            break;
        case JS_EVENT_AXIS:
            if (event.number < MAX_JOYSTICK_LENGTH) {
                m_raw.axes[event.number] = event.value;
            } else {
                tds_soft_error("exceeded MAX_JOYSTICK_LENGTH");
            }
            break;
        case JS_EVENT_INIT:
            break;
        }
        bytes = read(m_joystick_file, &event, sizeof(event));
    }
    
    if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return false; // data not yet available
        else {
            m_connected = false;
            tds_info("Joystick disconnected");
            return false;
        }
    }
    
    return true;
}

// When process_event fails, the previous raw values will be returned
Joystick_Raw Joystick::get_raw()
{
    process_event();
    return m_raw;
}

TDS_API Joystick_Raw joystick_raw(Joystick_ID joystick_id)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return {};
    
    return joystick->get_raw();
}

void Joystick::map_axis_idx_to_type(uint8_t axis_idx, Joystick_Axis axis_type) { m_axis_type_to_idx[axis_type] = axis_idx; }

TDS_API bool joystick_map_axis_idx_to_type(Joystick_ID joystick_id, uint8_t axis_idx, Joystick_Axis axis_type)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;
    
    joystick->map_axis_idx_to_type(axis_idx, axis_type);
    return true;
}

void Joystick::set_axis_range(Joystick_Axis axis_type, int16_t min, int16_t center, int16_t max)
{
    m_axis_min[axis_type] = min;
    m_axis_center[axis_type] = center;
    m_axis_max[axis_type] = max;
}

TDS_API bool joystick_set_axis_range(Joystick_ID joystick_id, Joystick_Axis axis_type, int16_t min, int16_t center, int16_t max)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;
    
    joystick->set_axis_range(axis_type, min, center, max);
    return true;
}

void Joystick::invert_axis(Joystick_Axis axis_type)
{
    int16_t temp = m_axis_min[axis_type];
    m_axis_min[axis_type] = m_axis_max[axis_type];
    m_axis_max[axis_type] = temp;
}

TDS_API bool joystick_invert_axis(Joystick_ID joystick_id, Joystick_Axis axis_type)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return false;
    
    joystick->invert_axis(axis_type);
    return true;    
}

// We map the raw value to [-1.0, 1.0] based on the raw min max and zero values.
// Including center ensures that the sticks resting position is actually mapped to zero.
// On the downside, the output won't be smove in 0, because we have two different
// linear interpolations for >= 0 and < 0.
Joystick_Data Joystick::get_data()
{
    process_event();

    Joystick_Data data;
    int16_t raw_value;
    
#define map_axis(axis_type, destination)                                \
    raw_value = m_raw.axes[m_axis_type_to_idx[(axis_type)]];            \
    if (raw_value < m_axis_center[(axis_type)]) {                       \
        (destination) = linear_map((double) raw_value,                  \
                                   (double) m_axis_min[(axis_type)],    \
                                   (double) m_axis_center[(axis_type)], \
                                   -1.0, 0.0);                          \
    }                                                                   \
    else {                                                              \
        (destination) = linear_map((double) raw_value,                  \
                                   (double) m_axis_center[(axis_type)], \
                                   (double) m_axis_max[(axis_type)],    \
                                   0.0, 1.0);                           \
    }

    map_axis(JOYSTICK_THROTTLE, data.throttle);
    map_axis(JOYSTICK_YAW, data.yaw);
    map_axis(JOYSTICK_PITCH, data.pitch);
    map_axis(JOYSTICK_ROLL, data.roll);
#undef map_axis
    
    return data;
}

TDS_API Joystick_Data joystick_data(Joystick_ID joystick_id)
{
    Joystick* joystick = g_objm.get_object(joystick_id);
    if (!joystick) return {};
    
    return joystick->get_data();
}
