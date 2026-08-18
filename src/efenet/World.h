#pragma once
#include <efenet/PlayerState.h>

#include <vector>

namespace efenet {

    // El estado authoritative: un array plano de jugadores. Nada mas.
    //
    // El servidor NO usa SceneGraph. La verdad del juego para avatares que
    // caminan son posiciones y angulos, no un grafo de render, y mantenerla
    // plana es lo que permite que efenet no linkee nada grafico.
    class World {
        public:
            // Devuelve kInvalidPlayer si ya hay kMaxPlayers.
            PlayerId Add(const glm::vec3& spawn);

            // false si el id no existe.
            bool Remove(PlayerId id);

            PlayerState*       Find(PlayerId id);
            const PlayerState* Find(PlayerId id) const;

            const std::vector<PlayerState>& Players() const { return m_players; }
            std::vector<PlayerState>&       Players()       { return m_players; }

            usize Count() const { return m_players.size(); }
            bool  Full()  const { return m_players.size() >= kMaxPlayers; }

        private:
            std::vector<PlayerState> m_players;

            // Los ids NO se reciclan: siempre sube. Reciclarlos haria que un
            // cliente con snapshots viejos en vuelo le atribuyera a un jugador
            // nuevo la posicion del que se fue.
            PlayerId m_nextId = 1u;
    };

}
