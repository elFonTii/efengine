#pragma once
#include <efengine/core/Types.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>
#include <efengine/serialization/StringTable.h>

#include <string_view>
#include <vector>

namespace efengine {
namespace serialization {

    // Los dos archivos que ve un componente al guardarse. Reexponen la misma
    // interfaz simetrica que BinaryWriter/BinaryReader (Field, Array, Count,
    // Ok, Version, kIsReading), asi que una sola funcion SerializeComponent
    // sirve para las dos direcciones, igual que con los behaviors.
    //
    // Lo que agregan es el contexto. Un BinaryWriter pelado alcanza para una
    // luz, pero no para una malla: hay que internar el path en la tabla de
    // strings del documento y traducir Model*/Material* a indices de
    // SceneAssets. Sin eso la malla no podria entrar por el registro y seguiria
    // siendo el caso especial que este diseno viene a sacar.

    struct ComponentWriter {
        static constexpr bool kIsReading = false;

        BinaryWriter&                     bin;
        StringTable&                      strings;
        const resources::SceneAssets&     assets;
        const resources::ResourceManager& rm;
        // Solo para los warnings: un "malla sin path" sin decir de que nodo no
        // sirve para nada. El componente no conoce el grafo, asi que el nombre
        // lo pone el serializador al armar el archivo.
        const char*                       nodeName = "";

        template <class T> void Field(T& value)             { bin.Field(value); }
        template <class T> void Array(std::vector<T>& v)    { bin.Array(v); }
        void Count(u32& value, usize minElementSize)        { bin.Count(value, minElementSize); }
        void Align4()                                       { bin.Align4(); }
        bool Ok()      const                                { return bin.Ok(); }
        u32  Version() const                                { return bin.Version(); }

        u32 Intern(std::string_view s) { return strings.Intern(s); }
    };

    struct ComponentReader {
        static constexpr bool kIsReading = true;

        BinaryReader&                bin;
        const StringTable&           strings;
        resources::SceneAssets&      assets;
        resources::ResourceManager&  rm;
        const MeshGeneratorRegistry& generators;
        const char*                  nodeName = "";   // solo para los warnings

        template <class T> void Field(T& value)             { bin.Field(value); }
        template <class T> void Array(std::vector<T>& v)    { bin.Array(v); }
        void Count(u32& value, usize minElementSize)        { bin.Count(value, minElementSize); }
        void Align4()                                       { bin.Align4(); }
        bool Ok()      const                                { return bin.Ok(); }
        u32  Version() const                                { return bin.Version(); }
        void Fail()                                         { bin.Fail(); }

        std::string_view View(u32 index) const { return strings.View(index); }
    };

}
}
