#pragma once
#include <efengine/core/Types.h>
#include <efengine/scene/ComponentStore.h>
#include <efengine/serialization/ComponentArchive.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace efengine {
namespace serialization {

    // El mismo puente que BehaviorRegistry, pero para los adjuntos de datos.
    // Register<T> genera los dos punteros a funcion que hacen falta y el
    // serializador recorre el registro: agregar un componente deja de tocar
    // SceneDocument y SceneSerializer.
    //
    // Dos diferencias con BehaviorRegistry, las dos a proposito:
    //
    //  * Las entries van en un vector, no en un unordered_map. El orden de
    //    emision tiene que ser estable o dos guardados de la misma escena dan
    //    bytes distintos (es la misma razon por la que los MaterialBinding se
    //    ordenan por nombre). El vector tambien evita el aliasing de punteros a
    //    nodos de mapa que hace BehaviorRegistry.
    //
    //  * No hay FindByType. Un componente no es polimorfico, no hay typeid
    //    dinamico que consultar: al guardar se recorre All() y cada entry
    //    pregunta si el nodo tiene SU tipo.
    class ComponentRegistry {
        public:
            struct Entry {
                std::string name;
                // false = el nodo no tiene este componente, o no hay que
                // emitirlo / adjuntarlo. El caller descarta el record.
                bool (*write)(ComponentWriter&, const scene::ComponentStore&, u32 nodeIndex) = null;
                bool (*read )(ComponentReader&, scene::ComponentStore&,       u32 nodeIndex) = null;
            };

            // T tiene que ser construible sin argumentos y tener una funcion
            // libre 'SerializeComponent(Ar&, T&)' visible por ADL.
            template <class T>
            void Register(const char* name) {
                static_assert(std::is_default_constructible_v<T>,
                              "ComponentRegistry::Register: T tiene que ser construible sin argumentos");

                Entry e;
                e.name = name;
                // Lambdas SIN captura: convierten a puntero a funcion.
                e.write = [](ComponentWriter& ar, const scene::ComponentStore& store, u32 nodeIndex) {
                    const T* c = store.TryGet<T>(nodeIndex);
                    if (c == null) return false;
                    // SerializeComponent es no-const porque una sola funcion
                    // cubre las dos direcciones; escribiendo no muta. Mismo
                    // criterio que el const superficial del unique_ptr en
                    // extractBehaviors.
                    return SerializeComponent(ar, const_cast<T&>(*c));
                };
                e.read = [](ComponentReader& ar, scene::ComponentStore& store, u32 nodeIndex) {
                    T value{};
                    if (!SerializeComponent(ar, value)) return false;
                    store.Set<T>(nodeIndex, std::move(value));
                    return true;
                };
                add(std::move(e));
            }

            // En orden de registro: es lo que hace deterministas los bytes.
            const std::vector<Entry>& All() const { return m_entries; }

            // Para cargar: el nombre que vino en el archivo.
            const Entry* FindByName(std::string_view name) const;

        private:
            void add(Entry entry);

            std::vector<Entry>                    m_entries;
            std::unordered_map<std::string, u32>  m_byName;   // nombre -> indice en m_entries
    };

}
}
