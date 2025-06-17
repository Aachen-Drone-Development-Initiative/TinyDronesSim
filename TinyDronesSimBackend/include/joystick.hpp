
#include "../tinydronessim_backend.hpp"

class Joystick {
public:
    static Joystick_ID create();
    static Joystick_ID create_and_connect(const char* device);
    ~Joystick();

    bool connect(const char* device);
    void disconnect();
    bool is_connected();
    Joystick_Raw get_raw();
    void map_axis_idx_to_type(uint8_t axis_idx, Joystick_Axis axis_type);
    void set_axis_range(Joystick_Axis axis_type, int16_t min, int16_t zero, int16_t max);
    void invert_axis(Joystick_Axis axis_type);
    Joystick_Data get_data();

private:
    Joystick() {};
    Joystick_ID m_id;

    bool process_event();

    bool m_connected = false;
    int m_joystick_file = -1;
    Joystick_Raw m_raw;

    uint8_t m_axis_type_to_idx[MAX_JOYSTICK_LENGTH];
    int16_t m_axis_min[MAX_JOYSTICK_LENGTH];
    int16_t m_axis_center[MAX_JOYSTICK_LENGTH];
    int16_t m_axis_max[MAX_JOYSTICK_LENGTH];
};
