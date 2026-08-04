#pragma once
#include <efengine/core/Types.h>

#include <memory>
#include <optional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace efengine {
namespace scene {

    // Los adjuntos de un nodo NO viven adentro del Node: viven aca, en un array
    // por tipo indexado por NodeHandle::index. Agregar un componente nuevo deja
    // de ser "otro campo en Node" (que engorda cada slot del grafo lo use o no)
    // y pasa a ser un array mas, que solo existe si alguien lo usa.
    //
    // Type-erased por afuera, tipado por adentro: el SceneGraph puede limpiar
    // todos los componentes de un indice sin saber que tipos hay, y el resto del
    // motor los pide por tipo concreto sin castear nada.
    //
    // OJO con las referencias: el puntero que devuelve TryGet apunta adentro de
    // un vector. Un Set/Remove sobre ESE tipo puede reallocarlo, y Clear los
    // invalida todos. No guardes el puntero mas alla del uso inmediato.
    class ComponentStore {
        public:
            // Adjunta (o pisa) el componente del nodo. Hace crecer el array si hace falta.
            template <class T>
            void Set(u32 nodeIndex, T value) {
                Array<T>& arr = findOrCreate<T>();
                if (nodeIndex >= arr.data.size()) arr.data.resize(nodeIndex + 1u);
                arr.data[nodeIndex] = std::move(value);
            }

            // Saca el componente del nodo. Si no lo tenia, no hace nada.
            template <class T>
            void Remove(u32 nodeIndex) {
                if (Array<T>* arr = find<T>()) arr->Reset(nodeIndex);
            }

            template <class T>
            T* TryGet(u32 nodeIndex) {
                Array<T>* arr = find<T>();
                if (arr == null || nodeIndex >= arr->data.size()) return null;
                std::optional<T>& slot = arr->data[nodeIndex];
                return slot ? &(*slot) : null;
            }

            template <class T>
            const T* TryGet(u32 nodeIndex) const {
                const Array<T>* arr = find<T>();
                if (arr == null || nodeIndex >= arr->data.size()) return null;
                const std::optional<T>& slot = arr->data[nodeIndex];
                return slot ? &(*slot) : null;
            }

            template <class T>
            bool Has(u32 nodeIndex) const { return TryGet<T>(nodeIndex) != null; }

            // Borra TODOS los componentes de ese indice, sin saber cuales son.
            // Lo llama el SceneGraph al destruir un nodo y al reusar un slot del
            // free list: sin esto los adjuntos de un nodo muerto sobrevivirian y
            // el proximo inquilino del indice los heredaria.
            void ResetNode(u32 nodeIndex) {
                for (auto& entry : m_arrays) entry.second->Reset(nodeIndex);
            }

            // Dimensiona los arrays para 'slotCount' nodos. No es obligatorio
            // (Set crece solo), pero evita una cadena de resizes chicos.
            void Reserve(usize slotCount) {
                if (slotCount <= m_slotCount) return;
                m_slotCount = slotCount;
                for (auto& entry : m_arrays) entry.second->Reserve(slotCount);
            }

            void Clear() {
                m_arrays.clear();
                m_slotCount = 0u;
            }

        private:
            struct IArray {
                virtual ~IArray() = default;
                virtual void Reset(u32 nodeIndex)     = 0;
                virtual void Reserve(usize slotCount) = 0;
            };

            template <class T>
            struct Array final : IArray {
                std::vector<std::optional<T>> data;

                void Reset(u32 nodeIndex) override {
                    if (nodeIndex < data.size()) data[nodeIndex].reset();
                }
                void Reserve(usize slotCount) override {
                    if (slotCount > data.size()) data.resize(slotCount);
                }
            };

            template <class T>
            Array<T>* find() {
                auto it = m_arrays.find(std::type_index(typeid(T)));
                // static_cast y no dynamic_cast: la clave del mapa ES el tipo,
                // asi que el array que hay en typeid(T) solo puede ser Array<T>.
                return (it != m_arrays.end()) ? static_cast<Array<T>*>(it->second.get()) : null;
            }

            template <class T>
            const Array<T>* find() const {
                auto it = m_arrays.find(std::type_index(typeid(T)));
                return (it != m_arrays.end()) ? static_cast<const Array<T>*>(it->second.get()) : null;
            }

            template <class T>
            Array<T>& findOrCreate() {
                std::unique_ptr<IArray>& slot = m_arrays[std::type_index(typeid(T))];
                if (!slot) {
                    auto arr = std::make_unique<Array<T>>();
                    arr->Reserve(m_slotCount);
                    slot = std::move(arr);
                }
                return *static_cast<Array<T>*>(slot.get());
            }

            std::unordered_map<std::type_index, std::unique_ptr<IArray>> m_arrays;
            usize m_slotCount = 0u;
    };

}
}
