#include <efenet/World.h>

namespace efenet {

    PlayerId World::Add(const glm::vec3& spawn) {
        if (Full()) return kInvalidPlayer;

        PlayerState p;
        p.id       = m_nextId++;
        p.position = spawn;
        p.yaw      = 0.0f;
        m_players.push_back(p);

        return p.id;
    }

    bool World::Remove(PlayerId id) {
        // Se itera con iteradores y no con indices para poder pasarle el
        // iterador a erase() sin castear el tipo de diferencia.
        for (auto it = m_players.begin(); it != m_players.end(); ++it) {
            if (it->id != id) continue;
            m_players.erase(it);
            return true;
        }
        return false;
    }

    PlayerState* World::Find(PlayerId id) {
        if (id == kInvalidPlayer) return null;
        for (PlayerState& p : m_players) {
            if (p.id == id) return &p;
        }
        return null;
    }

    const PlayerState* World::Find(PlayerId id) const {
        if (id == kInvalidPlayer) return null;
        for (const PlayerState& p : m_players) {
            if (p.id == id) return &p;
        }
        return null;
    }

}
