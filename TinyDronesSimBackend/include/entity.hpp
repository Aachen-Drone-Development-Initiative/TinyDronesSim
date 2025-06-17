#pragma once

#include "../tinydronessim_backend.hpp"

#include <utils/Entity.h>

class Environment;

class Entity {
public:
    Entity() : entity(), associated_env(nullptr) {}
    Entity(utils::Entity entity, Environment* associated_env) : entity(entity), associated_env(associated_env) {}

    bool is_valid() { return !entity.isNull(); }
    
    utils::Entity entity;
    Environment* associated_env;
};
