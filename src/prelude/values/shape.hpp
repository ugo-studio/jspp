#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <span>

namespace jspp {

class Shape {
public:
    // Map property name -> storage index
    std::unordered_map<std::string, uint32_t> property_offsets;
    // Map property name -> next Shape (transitions)
    std::unordered_map<std::string, std::shared_ptr<Shape>> transitions;
    
    // For fast enumeration (Object.keys)
    std::vector<std::string> property_names; 

    // Singleton empty shape
    static std::shared_ptr<Shape> empty_shape() {
        static auto shape = std::make_shared<Shape>();
        return shape;
    }

    std::optional<uint32_t> get_offset(const std::string& name) const {
        auto it = property_offsets.find(name);
        if (it != property_offsets.end()) return it->second;
        return std::nullopt;
    }

    std::shared_ptr<Shape> transition(const std::string& name) {
        auto it = transitions.find(name);
        if (it != transitions.end()) return it->second;

        // Create new shape
        auto new_shape = std::make_shared<Shape>();
        new_shape->property_offsets = this->property_offsets;
        new_shape->property_names = this->property_names;
        
        uint32_t new_offset = static_cast<uint32_t>(new_shape->property_offsets.size());
        new_shape->property_offsets[name] = new_offset;
        new_shape->property_names.push_back(name);

        transitions[name] = new_shape;
        return new_shape;
    }
};

}
