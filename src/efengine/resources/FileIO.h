#pragma once

#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <optional>
#include <string>


namespace efengine {
namespace resources {
    class FileIO {
       public:
        static std::optional<std::string> ReadText(const char* path);
    };
}
}