#pragma once
#include <stdint.h>

// Logica independiente del hardware. Un usuario por recorrido.
class ControlRecorrido {
public:
  enum Sentido { ESPERA, SUBIDA, BAJADA, FALLO };
  static const uint32_t INACTIVIDAD_MS = 180000UL;
  Sentido sentido = ESPERA;
  uint8_t fase = 0; // 0: antes del descanso, 1: despues, 2: llegada pendiente
  uint8_t pendientes[3] = {0, 0, 0};
  uint8_t cantidad = 0;
  uint32_t ultimaActividad = 0;

  void reiniciar() { sentido = ESPERA; fase = 0; cantidad = 0; }
  void fallo() { sentido = FALLO; fase = 0; cantidad = 0; }
  bool activo() const { return sentido == SUBIDA || sentido == BAJADA; }

  bool iniciar(Sentido nuevo, uint32_t ahora, bool descansoLibre,
               bool destinoLibre) {
    if (sentido != ESPERA || !descansoLibre || !destinoLibre) return false;
    if (nuevo != SUBIDA && nuevo != BAJADA) return false;
    sentido = nuevo; fase = 0; cantidad = 0; ultimaActividad = ahora;
    agregar(nuevo == SUBIDA ? 1 : 2);
    return true;
  }

  void sensor(uint8_t numero, uint32_t ahora) {
    if (!activo() || fase == 2) return;
    if (numero == 1 && fase == 0) {
      fase = 1; ultimaActividad = ahora; agregar(3); return;
    }
    uint8_t destino = sentido == SUBIDA ? 2 : 0;
    if (numero == destino && fase == 1) {
      fase = 2; ultimaActividad = ahora; agregar(sentido == SUBIDA ? 4 : 5);
    }
  }

  void repetir(uint32_t ahora, bool audioLibre) {
    if (activo() && fase != 2 && audioLibre && cantidad == 0) {
      agregar(sentido == SUBIDA ? 1 : 2); ultimaActividad = ahora;
    }
  }

  uint8_t extraer() {
    if (!cantidad) return 0;
    uint8_t pista = pendientes[0];
    for (uint8_t i = 1; i < cantidad; ++i) pendientes[i-1] = pendientes[i];
    --cantidad; return pista;
  }

  void audioTerminado(uint8_t pista) {
    if (fase == 2 && (pista == 4 || pista == 5)) reiniciar();
  }

  bool vencio(uint32_t ahora) {
    if (activo() && uint32_t(ahora - ultimaActividad) >= INACTIVIDAD_MS) {
      reiniciar(); return true;
    }
    return false;
  }
private:
  void agregar(uint8_t pista) {
    if (cantidad < 3) pendientes[cantidad++] = pista;
    else fallo();
  }
};
