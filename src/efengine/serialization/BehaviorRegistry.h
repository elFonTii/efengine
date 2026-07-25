#pragma once
#include <efengine/core/Types.h>
#include <efengine/scene/Behavior.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/serialization/BinaryWriter.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace efengine {
namespace serialization {

    // Los behaviors los define el cliente, no el motor: el motor no puede construir un
    // RotarY ni sabe que campos tiene. Este registry es el puente. Register<T> genera
    // los tres punteros a funcion que hacen falta, y typeid da la identidad al guardar.
    class BehaviorRegistry {
        public:
            struct Entry {
                std::string name;
                std::unique_ptr<scene::Behavior> (*create)() = null;
                void (*write)(BinaryWriter&, scene::Behavior&) = null;
                void (*read)(BinaryReader&, scene::Behavior&)  = null;
            };

            // T tiene que derivar de scene::Behavior, ser construible sin argumentos, y
            // tener 'template <class Ar> void Serialize(Ar&)'.
            template <class T>
            void Register(const char* name) {
                static_assert(std::is_base_of_v<scene::Behavior, T>,
                              "BehaviorRegistry::Register: T tiene que derivar de scene::Behavior");
                static_assert(std::is_default_constructible_v<T>,
                              "BehaviorRegistry::Register: T tiene que ser construible sin argumentos");

                Entry e;
                e.name   = name;
                // Lambdas SIN captura: convierten a puntero a funcion.
                e.create = []() -> std::unique_ptr<scene::Behavior> {
                    return std::make_unique<T>();
                };
                e.write  = [](BinaryWriter& w, scene::Behavior& b) {
                    static_cast<T&>(b).Serialize(w);
                };
                e.read   = [](BinaryReader& r, scene::Behavior& b) {
                    static_cast<T&>(b).Serialize(r);
                };
                add(std::type_index(typeid(T)), std::move(e));
            }

            // Para guardar: el tipo dinamico real de un Behavior.
            const Entry* FindByType(const scene::Behavior& behavior) const;
            // Para cargar: el nombre que vino en el archivo.
            const Entry* FindByName(std::string_view name) const;

        private:
            void add(std::type_index type, Entry entry);

            std::unordered_map<std::type_index, Entry>    m_byType;
            std::unordered_map<std::string, const Entry*> m_byName;
    };

}
}
