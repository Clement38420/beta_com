#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "library.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int test_cobs_encode_decode(void) {
    // 1. Données de test (mélange de valeurs et de zéros)
    uint8_t original[] = {0x11, 0x00, 0x22, 0x33, 0x00, 0x44};
    size_t original_len = sizeof(original);

    uint8_t encoded[256];
    size_t encoded_len = sizeof(encoded);

    uint8_t decoded[256];
    size_t decoded_len = sizeof(decoded);

    printf("--- TEST COBS ---\n");
    print_hex("Original", original, original_len);

    // 2. Test ENCODE
    if (cobs_encode(original, original_len, encoded, &encoded_len) != 0) {
        printf("Erreur: Echec de l'encodage\n");
        return 1;
    }
    print_hex("Encoded ", encoded, encoded_len);

    // Vérification basique du format COBS : le dernier octet doit être 0x00
    assert(encoded[encoded_len - 1] == 0x00);

    // 3. Test DECODE
    // Note: On passe encoded_len - 1 pour ne pas inclure le 0x00 final dans le décodage
    // si ta fonction s'arrête au pointeur de fin.
    if (cobs_decode(encoded, encoded_len - 1, decoded, &decoded_len) != 0) {
        printf("Erreur: Echec du décodage\n");
        return 1;
    }
    print_hex("Decoded ", decoded, decoded_len);

    // 4. Comparaison finale
    if (original_len == decoded_len && memcmp(original, decoded, original_len) == 0) {
        printf("\nRESULTAT: SUCCÈS (Les données sont identiques)\n");
    } else {
        printf("\nRESULTAT: ÉCHEC (Les données diffèrent)\n");
        return 1;
    }

    return 0;
}
