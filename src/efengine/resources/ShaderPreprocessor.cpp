#include "ShaderPreprocessor.h"

#include <efengine/core/Log.h>

#include <set>

namespace efengine {
namespace resources {

    namespace {

        // Si la linea es una directiva '#include "path"', devuelve el path.
        // Devuelve un optional vacio si no es una directiva.
        // outMalformed queda en true si arranca como include pero esta roto
        // (sin comillas): eso es un error, no "no es un include".
        std::optional<std::string> ParseIncludeLine(const std::string& line, bool& outMalformed) {
            outMalformed = false;

            usize i = line.find_first_not_of(" \t");
            if (i == std::string::npos) return std::nullopt;

            static const std::string kDirective = "#include";
            if (line.compare(i, kDirective.size(), kDirective) != 0) return std::nullopt;
            i += kDirective.size();

            const usize open = line.find('"', i);
            if (open == std::string::npos) { outMalformed = true; return std::nullopt; }

            const usize close = line.find('"', open + 1u);
            if (close == std::string::npos) { outMalformed = true; return std::nullopt; }

            return line.substr(open + 1u, close - open - 1u);
        }

        // Recursion con el set de ya-incluidos compartido (include-once).
        bool SpliceInto(const std::string& source, const IncludeResolver& resolver,
                        std::set<std::string>& seen, u32 depth, std::string& out) {
            if (depth > kMaxIncludeDepth) {
                EF_LOG_ERROR("SpliceIncludes: se excedio la profundidad maxima de include (%u)",
                             kMaxIncludeDepth);
                return false;
            }

            // pos < size y no <=: asi una fuente que termina en '\n' no produce una
            // linea vacia final. Preservar la fuente byte a byte es lo que evita que
            // los numeros de linea que reporta el compilador de GLSL se corran.
            usize pos = 0u;
            while (pos < source.size()) {
                usize eol = source.find('\n', pos);
                const bool lastLine = (eol == std::string::npos);
                if (lastLine) eol = source.size();

                const std::string line = source.substr(pos, eol - pos);

                bool malformed = false;
                const std::optional<std::string> path = ParseIncludeLine(line, malformed);

                if (malformed) {
                    EF_LOG_ERROR("SpliceIncludes: directiva de include mal formada: '%s'",
                                 line.c_str());
                    return false;
                }

                if (path.has_value()) {
                    if (seen.count(*path) == 0u) {
                        seen.insert(*path);

                        const std::optional<std::string> content = resolver(*path);
                        if (!content.has_value()) {
                            EF_LOG_ERROR("SpliceIncludes: no se pudo resolver el include '%s'",
                                         path->c_str());
                            return false;
                        }
                        if (!SpliceInto(*content, resolver, seen, depth + 1u, out)) return false;
                    }
                    // Ya visto: se emite una linea vacia para no correr los numeros
                    // de linea que reporta el compilador de GLSL mas de lo necesario.
                    // Y si se resolvio, el '\n' cierra la fuente incluida.
                    out += '\n';
                } else {
                    out += line;
                    // Sin '\n' si la fuente no lo tenia: la ultima linea sin salto
                    // se emite sin salto.
                    if (!lastLine) out += '\n';
                }

                if (lastLine) break;
                pos = eol + 1u;
            }

            return true;
        }

    }

    std::optional<std::string> SpliceIncludes(const std::string& source,
                                             const IncludeResolver& resolver) {
        std::set<std::string> seen;
        std::string out;
        out.reserve(source.size() * 2u);   // los includes casi siempre agrandan

        if (!SpliceInto(source, resolver, seen, 0u, out)) return std::nullopt;
        return out;
    }

}
}
