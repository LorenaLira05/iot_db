/*
 * ============================================================================
 *  PROJETO : Mangue Interativo — Cérebro ESP32-S3 Matrix
 *  VERSÃO  : 7.11 "Organismo Sereno"  (evolução da V7.1 / V7.0 / V6.0)
 *  PLACA   : Waveshare ESP32-S3-Matrix
 *            - Matriz WS2812B 8x8 ........ GPIO14
 *            - IMU QMI8658 (I2C) ......... SDA GPIO11 / SCL GPIO12, end. 0x6B
 * ----------------------------------------------------------------------------
 *  O QUE ESTA VERSÃO FAZ
 *   - Rosto PROCEDURAL: olhos (7 formas) + boca (9 formas) + sobrancelhas,
 *     combinados por expressão. Nada de bitmaps fixos por emoção.
 *   - Pupilas com física de mola-amortecedor ("olhos de boneco"):
 *       * inclinar a placa -> as pupilas "rolam" para o lado mais baixo
 *       * pessoa à frente (HC-SR04) -> olha em frente; muito perto -> vesgo
 *       * sem estímulo -> sacadas aleatórias (olhar à volta, como um ser vivo)
 *   - RESPIRAÇÃO (camada base): o rosto cresce e encolhe suavemente
 *     (reamostragem por área, sub-pixel). O brilho fica quase constante —
 *     na V7.0 o "coração" variava o brilho ~4x e cansava a vista.
 *     O ritmo muda com a emoção/energia sem saltos (acumulador de fase).
 *   - Cores com transição perceptual (OKLCh): sem escurecer nem acinzentar
 *     a meio da troca; transições lentas (≈1,2 s de constante de tempo).
 *   - Piscar de olhos DESLIGADO por defeito (PISCAR_OLHOS 1 para ligar).
 *   - BRILHO GERAL (padrão 40%) com slider na página Web, guardado na flash.
 *     Dithering temporal a 100 FPS para que o brilho baixo continue suave.
 *   - PALETAS de cor na página Web (5 predefinidas + "cor única" livre),
 *     guardadas na flash; a troca é animada em OKLCh.
 *   - A página Web assume o tema da paleta e mostra um ESPELHO AO VIVO da
 *     matriz (/api/matriz) + aviso quando um sensor em erro sobrepõe a cor.
 *   - ORDEM DAS CORES dos LEDs corrigida para RGB (esta matriz não é GRB:
 *     vermelho/verde saíam trocados) e ajustável na Web com "Testar cores".
 *   - Sensor de solo desligado já não põe o rosto em ERRO vermelho.
 *   - SINALIZAÇÃO DE SISTEMA redesenhada (secção 4b): canal próprio no canto
 *     da matriz + painel "Saúde do sistema" e histórico na Web. O rosto deixa
 *     de mudar de cor por causa de avarias.
 *   - TRÊS CAMADAS NA PLACA: (1) indicador no canto; (2) PAINEL DE ESTADO com
 *     4 ícones (ao ligar, toque no BOOT, botão Web, ou sozinho num erro novo);
 *     (3) manter o BOOT 5 s reabre a configuração Wi-Fi (anel de progresso).
 *   - ROTAÇÃO AUTOMÁTICA: com a placa de pé, o rosto (e o painel, o indicador
 *     e os avisos) gira 90/180/270° para ficar direito quando um lado está a
 *     < 15° do horizonte; giro animado; olhar e repouso acompanham.
 *   - MONTAGEM DO SENSOR corrigida (+90°, o QMI8658 está rodado face à matriz)
 *     e CALIBRAÇÃO GUIADA de orientação em 2 passos na página (distingue
 *     rotação de espelho).
 *   - GIRAR ↺↻ e FIXAR 🔒 (manipulação direta) na página e no BOOT (duplo
 *     toque = modo girar: toque gira 90°, segurar 1,5 s fixa/solta).
 *   - CORREÇÃO DO REINÍCIO EM CICLO (rst:0xc): show() da NeoPixel usa ~6 KB
 *     de pilha; agora corre a partir do loop(), com buffers estáticos e pilha
 *     de 16 KB. Diagnóstico de reinícios (motivo + módulo) e pilha livre.
 *   - ENERGIA interna (0-100): cai com o tempo e com sede, sobe com interação
 *     e repouso. Influencia humor, ritmo respiratório, brilho e piscar.
 *   - Despertar ao ligar (pálpebras abrem), adormecer por inatividade e
 *     PROTETOR DE ECRÃ animado (Maré, Jogo da Vida, Vaga-lumes), lento e suave.
 *   - Falhas (SEM piscar, canal próprio): indicador no canto da matriz
 *     (azul = a iniciar, âmbar = atenção, vermelho fixo = erro); Wi-Fi perdido
 *     -> CONFUSO 4 s + ícone Wi-Fi 1x (e de novo só quando alguém chega);
 *     acelerómetro em ERRO -> olhos em "X" e respiração ofegante.
 *   - Servidor Web + API JSON, arrancado só DEPOIS de o portal do
 *     WiFiManager libertar a porta 80.
 *
 *  REGRA DE OURO: zero delay(). Tudo é temporizado com millis().
 *  (A única espera é o pulso de 10 µs do HC-SR04 — exigência do sensor.)
 *
 *  Bibliotecas: Adafruit NeoPixel, WiFiManager (tzapu) >= 2.0.
 *  Core: arduino-esp32 2.x ou 3.x.
 * ----------------------------------------------------------------------------
 *  BOTÃO BOOT (já existe na placa): toque curto = painel de estado;
 *  duplo toque = modo girar (toque = girar 90°, segurar 1,5 s = fixar/soltar);
 *  manter 5 s = reabrir a configuração de Wi-Fi (largar antes cancela).
 * ----------------------------------------------------------------------------
 *  PRIMEIRA UTILIZAÇÃO — CHECKLIST DE CALIBRAÇÃO (Monitor Série a 115200)
 *   1. Deixe a placa PARADA ~1 s ao ligar: "[IMU] Repouso calibrado".
 *      (Mudou a montagem? Botão "Calibrar repouso" na página Web.)
 *   2. Página Web -> "Orientação do rosto" -> "Calibrar orientação (2 passos)":
 *      (1) placa de pé com o rosto direito; (2) rodada 90° para a esquerda.
 *      Corrige a montagem do acelerómetro (rotação OU espelho) e guarda-a.
 *      Se o rosto aparecer rodado mesmo deitado, ajuste ROTACAO_MATRIZ.
 *   3. Solo: anote os mV com o sensor ao ar (SOLO_MV_SECO) e em água
 *      (SOLO_MV_MOLHADO) — campo "mv" em /api/estado.
 *   4. HC-SR04: confirme "[US] HC-SR04 detetado". Sem sensor, nada a fazer.
 *   5. Sem sensor de solo ligado? USAR_SENSOR_SOLO 0 (um pino ADC solto
 *      lê valores aleatórios e pode inventar "sede").
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>          // guarda o brilho na flash (NVS)
#include <esp_system.h>           // esp_reset_reason(): porque é que a placa reiniciou
#include <esp_attr.h>             // RTC_NOINIT_ATTR: memória que sobrevive a um reinício
#if __has_include(<soc/gpio_reg.h>)
#include <soc/gpio_reg.h>         // GPIO_IN_REG (leitura do pino na ISR, sem flash)
#endif

// PILHA DO loop(): o padrão é 8 KB. A Adafruit NeoPixel (core 3.x / IDF 5)
// reserva NA PILHA um array de numBytes·8 itens RMT dentro de show():
// 64 LEDs × 3 × 8 × 4 B = 6 144 B. Somado ao resto, rebentava (reinício em
// ciclo, rst:0xc). Damos 16 KB ao loop e chamamos show() a partir do loop().
#ifdef SET_LOOP_TASK_STACK_SIZE
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif
#include <Adafruit_NeoPixel.h>
#include <math.h>

/*
 * ============================================================================
 *  1. CONFIGURAÇÃO (tudo o que se ajusta está aqui)
 * ============================================================================
 */
// ---------- Matriz
#define PINO_MATRIZ              14
#define LARGURA                   8
#define ALTURA                    8
#define NUM_LEDS                 (LARGURA * ALTURA)
#define ROTACAO_MATRIZ            0      // 0, 1 (90°), 2 (180°), 3 (270°)
// ORDEM DAS CORES dos LEDs. A matriz desta placa recebe os bytes em ordem RGB
// (ESPHome declara-a "type: RGB"; testes públicos usam NEO_RGB no GPIO14).
// Fitas WS2812 comuns usam GRB. Com a ordem errada, vermelho e verde trocam.
// Pode ser mudada na página Web (botão "Testar cores") e fica guardada.
#define ORDEM_COR_PADRAO          0      // índice em ORDENS_COR: 0 = RGB

// ---------- I2C / QMI8658 (integrado na placa)
#define PINO_I2C_SDA             11
#define PINO_I2C_SCL             12
#define FREQ_I2C_HZ          400000UL
#define TIMEOUT_I2C_MS           10      // teto do bloqueio se o barramento falhar
#define QMI_END_PRIMARIO       0x6B
#define QMI_END_ALTERNATIVO    0x6A
// ROTAÇÃO AUTOMÁTICA DO ROSTO: com a placa de pé, o rosto gira em passos de
// 90° para ficar sempre direito, quando um dos lados está a < 15° do horizonte.
#define AUTO_ROTACAO_PADRAO       1      // ligada (pode desligar-se na página Web)
#define LIMIAR_ROTACAO_GRAUS     15.0f   // janela de captura em torno de 0/90/180/270°
#define VERTICAL_MAX_GRAUS       40.0f   // só roda se a matriz estiver a <= 40° da vertical
#define ROTACAO_ESTAVEL_MS      400      // a nova posição tem de se manter (evita oscilar)
#define DURACAO_ANIM_ROTACAO_MS 450      // o rosto gira suavemente até à nova posição
// MONTAGEM DO ACELERÓMETRO face à matriz (correção aplicada em tempo real).
// Nesta placa o QMI8658 está a 90° da matriz (exemplos públicos trocam X/Y;
// teste na placa real: sem correção o rosto ficava sempre 90° anti-horário).
// A calibração guiada na página Web mede a montagem real e guarda-a.
#define CORRECAO_EIXOS_PADRAO     1      // quartos de volta: 0..3 (1 = +90°)
#define ESPELHO_EIXOS_PADRAO      0      // 1 = um eixo invertido (montagem espelhada)
// Ajustes manuais adicionais (aplicados ANTES da correção acima):
#define EIXO_TROCAR_XY            0
#define EIXO_INVERTER_X           0
#define EIXO_INVERTER_Y           0

// ---------- HC-SR04 (opcional; deteção automática)
// ATENÇÃO: o ECHO do HC-SR04 sai a 5 V. O ESP32-S3 NÃO tolera 5 V.
// Use divisor resistivo (ex.: 1 kΩ série + 2 kΩ para GND) no pino ECHO.
#define USAR_ULTRASSOM            1
#define PINO_US_TRIG              5      // confirme no pinout os GPIO livres
#define PINO_US_ECHO              6
#define US_INTERVALO_MS          70      // fabricante: ciclo >= 60 ms
#define US_INTERVALO_SONDA_MS  2000      // se ausente, sonda devagar
#define US_TIMEOUT_MS            60
#define US_FALHAS_AUSENTE        10
#define US_PERTO_ENTRA_CM        60.0f
#define US_PERTO_SAI_CM          75.0f
#define US_VESGO_CM              12.0f
#define US_VARIACAO_MIN_CM        1.0f   // pessoas mexem-se; paredes não
#define US_OBJETO_ESTATICO_MS 45000      // sem variação -> é mobília, ignora

// ---------- Humidade do solo (analógico)
#define USAR_SENSOR_SOLO          1      // 0 se não estiver ligado (pino flutua!)
#define PINO_SENSOR_SOLO          4      // GPIO4 = ADC1 (ADC1 funciona com Wi-Fi)
#define SOLO_MV_SECO           2800      // CALIBRAR: sensor ao ar
#define SOLO_MV_MOLHADO        1200      // CALIBRAR: sensor em água
#define SOLO_PCT_ENTRA_SEDE      30
#define SOLO_PCT_SAI_SEDE        40
#define SOLO_AMOSTRAS             8

// ---------- Rede
#define PORTA_SERVIDOR_WEB       80
#define NOME_AP_CONFIG   "Mangue_Interativo_AP"
#define NOME_MDNS        "mangue"        // http://mangue.local

// ---------- Temporização (ms)
#define INTERVALO_FRAME_MS       10      // 100 FPS: o dithering alterna acima de ~50 Hz
                                         // (show() de 64 LEDs ≈ 2 ms por frame)
#define USAR_DITHERING            1      // níveis intermédios em brilho baixo (ver compositor)
#define INTERVALO_IMU_MS         10      // 100 Hz (ODR do sensor: 125 Hz)
#define INTERVALO_SOLO_MS      1000
#define INTERVALO_ENERGIA_MS   1000
#define INTERVALO_REDE_MS       500
#define INTERVALO_LOG_MS       5000
#define ESPERA_RESET_QMI_MS      20
#define BACKOFF_INICIAL_MS      500
#define BACKOFF_MAXIMO_MS      8000
#define FALHAS_PARA_ERRO          3

// ---------- Motor visual
#define BRILHO_BASE            0.22f     // fração de 255 (constante; sem pulsar)
#define MODULACAO_BRILHO       0.03f     // ±3% acompanhando a respiração (quase impercetível)
#define COMPENSAR_AREA         1.0f      // 0..1: 1 = repõe toda a luz perdida ao encolher (área ∝ s²)
#define BRILHO_PROTETOR        0.18f
#define BRILHO_AVISO           0.22f
// BRILHO GERAL: multiplica TUDO (rosto, avisos, protetor). Ajustável na página
// Web (5-100%) e guardado na flash. 40% ≈ menos de metade da luz da V7.1.
#define BRILHO_GERAL_PADRAO      40      // %
#define BRILHO_GERAL_MIN          5
#define BRILHO_GERAL_MAX        100
#define DURACAO_CROSSFADE_MS    900      // troca de rosto lenta
#define TAU_COR_MS            1200.0f    // troca de cor lenta (constante de tempo)
#define TAU_RITMO_MS          3000.0f    // ritmo respiratório muda devagar
#define TAU_AMPLITUDE_MS      2000.0f
#define PISCAR_OLHOS              0      // 1 = pestanejar ocasional (4-9 s, simples)
#define DURACAO_PISCAR_MS       220
#define DURACAO_ACORDAR_MS     1400
#define DURACAO_ADORMECER_MS   2200
#define DURACAO_FADE_PROT_MS   1500
#define DURACAO_CADA_PROT_MS  40000
#define DURACAO_AVISO_MS       4000

// ---------- Sinalização de SISTEMA (separada das emoções — ver secção 4b)
#define INDICADOR_X               7      // canto inferior direito: livre em todos os rostos
#define INDICADOR_Y               7
#define BRILHO_INDICADOR       0.25f     // ≈ pico do rosto: visível mas periférico
#define PERIODO_PULSO_IND_MS   4000      // pulso lento (fade), nunca pisca
#define INTERVALO_SAUDE_MS      250
#define WIFI_AVISO_APOS_MS     5000      // quedas de Wi-Fi mais curtas não alarmam
#define DURACAO_REACAO_MS      4000      // a planta reage (CONFUSO) e depois acalma
#define RELEMBRAR_APOS_MS     60000      // re-mostrar o ícone a quem chega: máx. 1x/min
#define MAX_EVENTOS              12      // histórico para a página Web

// ---------- Painel de estado na matriz (camada 2: "o quê", a pedido)
#define BRILHO_PAINEL          0.22f
#define DURACAO_PAINEL_MS      6000      // toque no BOOT / botão da página
#define DURACAO_PAINEL_ERRO_MS 5000      // aparece sozinho quando surge um erro novo
#define ARRANQUE_MIN_MS        2500      // verificação inicial ao ligar
#define ARRANQUE_MAX_MS        6000
#define ARRANQUE_ESCALONADO_MS  250      // ícones acendem um a um ("checklist")

// ---------- Botão BOOT (GPIO0: HIGH solto, LOW premido) — camada 3: agir
#define PINO_BOTAO                0
#define BOTAO_DEBOUNCE_MS        30
#define BOTAO_CURTO_MAX_MS      800      // < 0,8 s = toque curto -> painel
#define BOTAO_LONGO_MS         5000      // 5 s a segurar -> reabrir configuração Wi-Fi
#define PORTAL_TIMEOUT_S        180      // portal pedido pelo botão fecha ao fim de 3 min
#define DUPLO_TOQUE_MS          350      // 2 toques nesta janela = "modo girar"
#define MODO_GIRAR_INATIVO_MS 10000      // sai sozinho do modo girar
#define SEGURAR_FIXAR_MS       1500      // no modo girar: segurar = fixar/soltar
#define DURACAO_ICONE_MS       1200      // ícones de confirmação (girar, cadeado)

// ---------- Física das pupilas (mola-amortecedor)
#define PUPILA_RIGIDEZ          14.0f    // k   (1/s²)  -> movimento lento e fluido
#define PUPILA_AMORTECIMENTO     6.5f    // c   (1/s)   -> ζ ≈ 0.87, quase sem ressalto
#define PUPILA_GRAVIDADE        30.0f    // ganho da inclinação (unid. olho/s² por g)

// ---------- Movimento / interação
#define INCLINACAO_ENTRA        0.40f    // g (~24°) — histerese
#define INCLINACAO_SAI          0.28f    // g (~16°)
#define TAU_GRAVIDADE_MS        80.0f
#define LIMIAR_INTERACAO_G      0.08f
#define LIMIAR_SUSTO_G          0.80f
#define AMOSTRAS_CALIBRACAO      50
#define REFRATARIO_SUSTO_MS     800
#define DURACAO_SUSTO_MS       3000
#define SUSTOS_PARA_RAIVA         3
#define JANELA_RAIVA_MS       10000
#define DURACAO_RAIVA_MS       6000
#define DURACAO_ALIVIO_MS      2500
#define INTERACAO_RECENTE_MS  20000
#define AMOR_APOS_MS           8000
#define TEMPO_PROTETOR_MS     90000
#define DURACAO_MANUAL_S         10

// ---------- Energia (unidades: pontos de 0 a 100)
#define ENERGIA_INICIAL         70.0f
#define ENERGIA_DECAI_MIN        1.0f
#define ENERGIA_GANHO_INTER_S    0.6f
#define ENERGIA_DRENO_SEDE_MIN   2.0f
#define ENERGIA_REPOUSO_MIN      3.0f
#define ENERGIA_CUSTO_SUSTO      4.0f
#define ENERGIA_LIMIAR_SONO     20.0f

/*
 * ============================================================================
 *  2. TIPOS E ESTADO GLOBAL
 * ============================================================================
 */
// Emoções (as 6 originais + 3 novas, exigidas pelos novos comportamentos)
enum class Emocao : uint8_t {
  ESPERA, ALEGRIA, SEDE, SUSTO, AMOR, RAIVA, SONO, CURIOSO, CONFUSO, TOTAL
};
enum class StatusHardware : uint8_t { TENTANDO, SUCESSO, ERRO };
enum class ModoEcra       : uint8_t { ACORDAR, ROSTO, ADORMECER, PROTETOR };
enum class EstadoRede     : uint8_t { A_LIGAR, PORTAL, LIGADO, PERDIDO };
enum class TipoProtetor   : uint8_t { MARE, VIDA, VAGALUMES, TOTAL };

// ---- Saúde do sistema: UM modelo para todos os componentes -------------------
// AUSENTE != ERRO: um sensor opcional não instalado não é uma avaria.
enum class Saude : uint8_t { OK, A_LIGAR, AUSENTE, DESATIVADO, AVISO, ERRO };
enum class Nivel : uint8_t { OK, INFO, AVISO, ERRO };       // o que o indicador mostra
enum class Sub   : uint8_t { IMU, WIFI, SOLO, ULTRASSOM, TOTAL };
const char* const NOMES_SUB[]   = {"imu", "wifi", "solo", "ultrassom"};
const char* const NOMES_SAUDE[] = {"ok", "a_ligar", "ausente", "desativado", "aviso", "erro"};
const char* const NOMES_NIVEL[] = {"ok", "info", "aviso", "erro"};
struct EstadoSub { Saude saude = Saude::A_LIGAR; unsigned long desde = 0; };
struct Evento    { unsigned long t; uint8_t sub; Saude de, para; };

// ---- Geometria do rosto -----------------------------------------------------
// Olho = caixa 3x3. Definido para o olho ESQUERDO (bit2 = coluna exterior);
// o direito é espelhado automaticamente.
enum class FormaOlho : uint8_t { NORMAL, FELIZ, SEMICERRADO, FECHADO, CORACAO, X, ZANGADO, TOTAL };
struct DefOlho { uint8_t linhas[3]; bool temPupila; bool pestaneja; };

const DefOlho DEF_OLHOS[] = {
  /* NORMAL      */ {{0b111, 0b111, 0b111}, true,  true },
  /* FELIZ  ^ ^  */ {{0b010, 0b101, 0b000}, false, false},
  /* SEMICERRADO */ {{0b000, 0b111, 0b111}, true,  true },
  /* FECHADO     */ {{0b000, 0b000, 0b111}, false, false},
  /* CORACAO     */ {{0b101, 0b111, 0b010}, false, false},
  /* X           */ {{0b101, 0b010, 0b101}, false, false},
  /* ZANGADO \ / */ {{0b100, 0b110, 0b111}, true,  true },
};
static_assert(sizeof(DEF_OLHOS) / sizeof(DEF_OLHOS[0]) == (size_t)FormaOlho::TOTAL, "DEF_OLHOS incompleto");

// Boca = linhas 5, 6 e 7 (bit7 = coluna 0)
enum class FormaBoca : uint8_t { NEUTRA, SORRISO, TRISTE, O, ONDULADA, DENTES, PEQUENA, TORTA, BEIJO, TOTAL };
const uint8_t DEF_BOCAS[][3] = {
  /* NEUTRA   */ {0b00000000, 0b00111100, 0b00000000},
  /* SORRISO  */ {0b10000001, 0b01000010, 0b00111100},
  /* TRISTE   */ {0b00000000, 0b00111100, 0b01000010},
  /* O        */ {0b00011000, 0b00100100, 0b00011000},
  /* ONDULADA */ {0b00000000, 0b01010101, 0b10101010},
  /* DENTES   */ {0b01111110, 0b01011010, 0b01111110},
  /* PEQUENA  */ {0b00000000, 0b00011000, 0b00000000},
  /* TORTA    */ {0b00000000, 0b00001110, 0b01110000},
  /* BEIJO    */ {0b00000000, 0b00100100, 0b00011000},
};
static_assert(sizeof(DEF_BOCAS) / sizeof(DEF_BOCAS[0]) == (size_t)FormaBoca::TOTAL, "DEF_BOCAS incompleto");

#define LINHA_SOBRANCELHAS 0
#define LINHA_OLHOS        1
#define LINHA_BOCA         5

// ---- Expressões: uma linha por emoção (data-driven, sem if/switch) --------
struct Expressao {
  const char* nome;
  FormaOlho   olhoEsq, olhoDir;
  FormaBoca   boca;
  uint8_t     sobrancelhas;       // linha 0 (bit7 = coluna 0)
  uint8_t     r, g, b;            // cor em estado SUCESSO
  float       respMin;            // respirações por minuto (ritmo do "tamanho")
  float       amplitude;          // 0..0.2: quanto o rosto encolhe ao expirar
  float       intensidade;        // 0..1 multiplicador do brilho base
};

const Expressao EXPRESSOES[] = {
  // nome       olhoE                   olhoD                   boca                sobr.        R    G    B  resp/min ampl.  intens.
  {"espera",  FormaOlho::NORMAL,      FormaOlho::NORMAL,      FormaBoca::NEUTRA,   0,          90,  90, 110,  10, 0.08f, 0.80f},
  {"alegria", FormaOlho::FELIZ,       FormaOlho::FELIZ,       FormaBoca::SORRISO,  0,           0, 170,  30,  14, 0.10f, 1.00f},
  {"sede",    FormaOlho::SEMICERRADO, FormaOlho::SEMICERRADO, FormaBoca::ONDULADA, 0,         170,  80,   0,   8, 0.06f, 0.80f},
  {"susto",   FormaOlho::NORMAL,      FormaOlho::NORMAL,      FormaBoca::O,        0b11100111, 120,   0, 200,  24, 0.10f, 1.00f},
  {"amor",    FormaOlho::CORACAO,     FormaOlho::CORACAO,     FormaBoca::BEIJO,    0,         220,  20, 100,  12, 0.12f, 1.00f},
  {"raiva",   FormaOlho::ZANGADO,     FormaOlho::ZANGADO,     FormaBoca::DENTES,   0,         220,  40,   0,  22, 0.10f, 1.00f},
  {"sono",    FormaOlho::FECHADO,     FormaOlho::FECHADO,     FormaBoca::PEQUENA,  0,          40,  50, 140,   6, 0.06f, 0.60f},
  {"curioso", FormaOlho::NORMAL,      FormaOlho::NORMAL,      FormaBoca::PEQUENA,  0,           5, 173,  96,  16, 0.08f, 0.95f},
  {"confuso", FormaOlho::NORMAL,      FormaOlho::SEMICERRADO, FormaBoca::TORTA,    0b11100000, 160, 140,   0,  12, 0.08f, 0.90f},
};
static_assert(sizeof(EXPRESSOES) / sizeof(EXPRESSOES[0]) == (size_t)Emocao::TOTAL, "EXPRESSOES incompleto");

// Cores de estado de hardware (sobrepõem a cor da emoção)
const uint8_t COR_TENTANDO[3] = {0, 150, 200};   // ciano: "a ligar sensores"
const uint8_t COR_ERRO[3]     = {200, 0, 0};     // vermelho: falha de sensor

// ---- PALETAS (escolhidas na página Web) ------------------------------------
// Uma cor por emoção, na ordem do enum Emocao. Regras de desenho seguidas:
//  - TEMA FORTE: ESPERA e ALEGRIA (as mais frequentes) a ΔE >= 0,15 da
//    paleta original — senão trocar de paleta "não se nota" (bug da V7.3);
//  - quaisquer duas emoções a ΔE(OKLab) >= 0,085 (distinguíveis numa 8x8);
//  - todas a ΔE >= 0,11 do vermelho de ERRO e do ciano de TENTANDO, para o
//    feedback de hardware nunca se confundir com uma emoção.
//  (Valores obtidos por busca de alteração mínima; ver banco_de_testes.)
// A paleta 0 ("mangue") usa as cores da tabela EXPRESSOES.
enum class Paleta : uint8_t { MANGUE, OCEANO, POR_DO_SOL, PASTEL, FLORESTA, COR_UNICA, TOTAL };
const char* const NOMES_PALETA[] = {"mangue", "oceano", "por_do_sol", "pastel", "floresta", "cor_unica"};
static_assert(sizeof(NOMES_PALETA) / sizeof(NOMES_PALETA[0]) == (size_t)Paleta::TOTAL, "NOMES_PALETA incompleto");

const uint8_t CORES_PALETA[][(size_t)Emocao::TOTAL][3] = {
  // espera         alegria        sede           susto          amor           raiva          sono           curioso        confuso
  {{0,0,0}},  // MANGUE -> usa EXPRESSOES (marcador)
  {{4,99,254}, {0,218,175}, {200,170,90}, {140,80,255}, {230,90,200}, {255,90,70}, {30,40,140}, {137,233,255}, {149,167,186}},  // OCEANO
  {{255,143,81}, {255,190,30}, {186,92,22}, {200,40,160}, {255,60,120}, {255,55,0}, {110,40,140}, {255,230,120}, {192,128,146}},  // POR_DO_SOL
  {{155,189,232}, {150,220,106}, {234,179,129}, {188,112,254}, {255,151,241}, {255,122,123}, {110,140,220}, {130,255,227}, {255,248,156}},  // PASTEL
  {{107,166,93}, {93,242,63}, {196,140,37}, {150,90,200}, {230,90,130}, {210,89,20}, {40,80,70}, {178,228,106}, {160,119,87}},  // FLORESTA
};
#define COR_UNICA_PADRAO   0xFFA02Eu      // âmbar (bem diferente da paleta original)
#define COR_MIN_CANAL_MAX  40             // cor mais escura aceite (senão o rosto "some")
const uint8_t COR_PORTAL[3]   = {0, 150, 200};
const uint8_t COR_PERDIDO[3]  = {220, 110, 0};

// Ícones de orientação (8x8, bit7 = coluna 0)
const uint8_t ICONE_GIRAR[8]   = {0b00111010, 0b01000110, 0b10001110, 0b10000000, 0b10000001, 0b10000001, 0b01000010, 0b00111100};  // ↻
const uint8_t ICONE_FIXO[8]    = {0b00111100, 0b01000010, 0b01000010, 0b11111111, 0b11100111, 0b11100111, 0b11111111, 0b00000000};  // cadeado fechado
const uint8_t ICONE_LIVRE[8]   = {0b00111100, 0b01000010, 0b00000010, 0b11111111, 0b11100111, 0b11100111, 0b11111111, 0b00000000};  // cadeado aberto

// Ícone Wi-Fi: 3 camadas (ponto, arco médio, arco grande) para animar
const uint8_t ICONE_WIFI[3][8] = {
  {0, 0, 0, 0, 0, 0b00011000, 0b00011000, 0},                       // ponto
  {0, 0, 0, 0b00111100, 0b01000010, 0, 0, 0},                       // arco médio
  {0, 0b01111110, 0b10000001, 0, 0, 0, 0, 0},                       // arco grande
};

// ---- Estado: sensores --------------------------------------------------------
struct EstadoIMU {
  enum class Fase : uint8_t { PROCURAR, AGUARDAR_RESET, ATIVO } fase = Fase::PROCURAR;
  StatusHardware status = StatusHardware::TENTANDO;
  uint8_t  endereco = 0;
  uint8_t  falhasSeguidas = 0;
  uint32_t backoffMs = BACKOFF_INICIAL_MS;
  unsigned long tProximaAcao = 0;
  unsigned long tUltimaLeitura = 0;
  float gravX = 0, gravY = 0, gravZ = 1;   // componente lenta (gravidade), em g — já corrigida
  float cruX = 0, cruY = 0, cruZ = 1;      // o mesmo, SEM a correção de montagem (para calibrar)
  float refX = 0, refY = 0, refZ = 1;      // orientação de repouso (calibrada)
  bool  calibrado = false;
  uint16_t amostrasCal = 0;
  float agitacao = 0;                      // |a - gravidade| (g), com decaimento
  uint32_t leiturasOk = 0, leiturasFalhadas = 0;
} imu;

struct MontagemEixos {
  uint8_t quarto = CORRECAO_EIXOS_PADRAO;  // rotação dos eixos do sensor: quarto × 90°
  bool espelho = ESPELHO_EIXOS_PADRAO;
  // calibração guiada (2 passos)
  bool passo1Feito = false;
  float phiPasso1 = 0;                     // ângulo cru com a placa direita
} eixos;

struct EstadoRotacao {
  bool  automatica = AUTO_ROTACAO_PADRAO;
  uint8_t quarto = 0;                      // 0..3 -> 0/90/180/270° (orientação estável)
  float anguloExibido = 0;                 // ângulo usado para desenhar (anima entre quartos)
  float anguloInicio = 0, anguloAlvo = 0;
  unsigned long tInicioAnim = 0;
  bool  animando = false;
  int8_t candidato = -1;                   // quarto à espera de estabilizar
  unsigned long tCandidato = 0;
  float anguloMedido = 0;                  // direção da gravidade no plano (graus)
  bool  vertical = false;                  // matriz de pé o suficiente para rodar?
  uint8_t quartoFixo = 0;                  // posição guardada quando está FIXA (cadeado)
} rot;

struct EstadoMovimento {
  bool  emMovimento = false;
  float inclX = 0, inclY = 0;              // desvio da orientação de repouso (g)
} mov;

struct EstadoUltrassom {
  bool  detectado = false;                 // sensor existe e responde?
  bool  decidido = false;                  // já sabemos se existe? (até lá: "a verificar")
  bool  aguardarEco = false;
  uint8_t semResposta = 0;
  unsigned long tDisparo = 0;
  float janela[3] = {999, 999, 999};       // mediana de 3
  uint8_t idx = 0;
  float distCm = 999;                      // 999 = nada à frente
  float distRef = 999;                     // referência p/ velocidade
  unsigned long tRef = 0;
  bool  pessoaPerto = false;
  unsigned long tPessoaDesde = 0;
  float distUltima = 999;                  // p/ distinguir pessoa de parede
  unsigned long tUltimaVariacao = 0;
} us;

volatile unsigned long ecoInicioUs = 0, ecoDuracaoUs = 0;
volatile bool ecoViuSubida = false, ecoPronto = false;

struct EstadoSolo {
  StatusHardware status = StatusHardware::TENTANDO;
  bool  presente = true;                   // false: leituras fora de gama (sem sensor)
  bool  temLeitura = false;
  float mvFiltrado = 0;
  int   percentagem = -1;
  bool  comSede = false;
  uint8_t leiturasForaGama = 0;
} solo;

// ---- Estado: comportamento ---------------------------------------------------
struct EstadoEmocao {
  Emocao atual = Emocao::ESPERA;
  bool transitoriaAtiva = false; Emocao transitoria = Emocao::ESPERA; unsigned long fimTransitoria = 0;
  bool manualAtiva = false;      Emocao manual = Emocao::ESPERA;      unsigned long fimManual = 0;
  unsigned long tUltimoSusto = 0, inicioJanelaRaiva = 0;
  uint8_t sustosNaJanela = 0;
  unsigned long tUltimaInteracao = 0;
  float energia = ENERGIA_INICIAL;
} emo;

EstadoSub subs[(uint8_t)Sub::TOTAL];
Evento eventos[MAX_EVENTOS];
uint8_t nEventos = 0, iEvento = 0;       // buffer circular
unsigned long tSaude = 0;

struct EstadoPainel {
  bool ativo = false;
  bool arranque = false;                   // verificação inicial ao ligar
  unsigned long tInicio = 0, duracao = 0;
  unsigned long tTudoResolvido = 0;        // arranque: quando nenhum ficou "a ligar"
  unsigned long tUltimoAuto = 0;           // última vez que abriu sozinho
  int8_t destaque = -1;                    // componente culpado (-1 = nenhum)
} painel;

struct EstadoBotao {
  bool leituraAnterior = false, premido = false, longoFeito = false;
  unsigned long tMudanca = 0, tPressao = 0;
  bool toquePendente = false;              // 1.º toque à espera de um possível 2.º
  unsigned long tToque = 0;
  bool modoGirar = false;                  // duplo toque: toque = girar, segurar = fixar
  unsigned long tUltimaAcaoGirar = 0;
} botao;

// Ícone de confirmação breve (girar ↻, cadeado) — feedback imediato na placa
struct IconeBreve { const uint8_t* bmp = nullptr; uint8_t cor[3] = {0, 0, 0}; unsigned long tInicio = 0; } iconeBreve;
void mostrarIconeBreve(const uint8_t* bmp, uint8_t r, uint8_t g, uint8_t b, unsigned long agora) {
  iconeBreve.bmp = bmp; iconeBreve.cor[0] = r; iconeBreve.cor[1] = g; iconeBreve.cor[2] = b; iconeBreve.tInicio = agora;
}

void abrirPainel(unsigned long agora, unsigned long duracao, int8_t destaque) {
  painel.ativo = true;
  painel.arranque = false;
  painel.tInicio = agora;
  painel.duracao = duracao;
  painel.destaque = destaque;
}

struct EstadoConexao {
  EstadoRede estado = EstadoRede::A_LIGAR;
  bool jaLigou = false;
  unsigned long tPerda = 0;
  bool servidorAtivo = false;
} rede;

// ---- Estado: visual ----------------------------------------------------------
struct EstadoOlhos {
  float px = 0, py = 0, vx = 0, vy = 0;    // posição/velocidade da pupila (-1..1)
  float alvoX = 0, alvoY = 0;
  float convergencia = 0;                  // >0 = vesgo
  unsigned long tProximaSacada = 0;
  unsigned long tInicioPiscar = 0, tProximoPiscar = 0;
  bool  aPiscar = false;
  uint8_t piscarPendente = 0;              // para piscar duplo
  uint16_t duracaoPiscar = DURACAO_PISCAR_MS;
} olhos;

struct EstadoVisual {
  ModoEcra modo = ModoEcra::ACORDAR;
  unsigned long tModo = 0;
  float alfaProtetor = 0;
  Emocao emocaoAnterior = Emocao::ESPERA;
  unsigned long tInicioCrossfade = 0;
  float corLch[3] = {0.70f, 0.12f, 3.6f};      // cor atual em OKLCh (L, C, h rad)
  float fase = 0, ritmo = 10, amplitude = 0.08f;
  float escala = 1.0f;
  unsigned long tUltimoFrame = 0;
  unsigned long tInicioAviso = 0, tUltimoAviso = 0;
  bool avisoAtivo = false;
  bool avisoPendente = false;              // ícone Wi-Fi à espera de ser mostrado
} vis;

struct Vagalume { float ax, ay, fx, fy, fase, freqPisca, fasePisca; };
struct EstadoProtetor {
  TipoProtetor tipo = TipoProtetor::MARE;
  unsigned long tInicio = 0;
  float rgb[NUM_LEDS][3];
  uint8_t vida[NUM_LEDS];
  float brilhoVida[NUM_LEDS];
  uint16_t idadeVida[NUM_LEDS];
  uint32_t assinaturas[2];
  uint16_t geracoes = 0;
  unsigned long tPassoVida = 0;
  Vagalume vag[5];
} prot;

struct Diagnostico {
  unsigned long tJanela = 0;
  uint32_t loopMaxUs = 0, loopMaxUsPublicado = 0;
  uint32_t frames = 0, fps = 0;
  uint32_t pilhaLivreMin = 0;             // bytes livres mínimos na pilha do loop (medido)
} diag;

/*
 *  DIAGNÓSTICO DE REINÍCIOS — "migalhas" numa zona de RAM que sobrevive a um
 *  reinício por software/crash (RTC_NOINIT). Antes de cada módulo, o loop grava
 *  em que etapa está; se a placa rebentar, no arranque seguinte sabemos ONDE.
 *  Também conta crashes seguidos. Mostrado no Monitor Série e na página.
 */
enum class Etapa : uint8_t { ARRANQUE, REDE, BOTAO, IMU, ULTRASSOM, SOLO, SAUDE, ENERGIA, EMOCAO, RENDER, LEDS, DIAGNOSTICO, TOTAL };
const char* const NOMES_ETAPA[] = {"arranque", "rede/web", "botao", "acelerometro", "ultrassom", "solo", "saude", "energia", "emocao", "desenho", "envio aos LEDs", "diagnostico"};
static_assert(sizeof(NOMES_ETAPA) / sizeof(NOMES_ETAPA[0]) == (size_t)Etapa::TOTAL, "NOMES_ETAPA incompleto");
#define MAGIA_RTC 0x4D414E47u            // "MANG"
struct MigalhasRTC { uint32_t magia; uint8_t etapa; uint8_t crashesSeguidos; };
RTC_NOINIT_ATTR MigalhasRTC migalhas;
struct InfoReinicio {
  esp_reset_reason_t motivo = ESP_RST_UNKNOWN;
  bool foiCrash = false;
  Etapa etapaCrash = Etapa::ARRANQUE;
  uint8_t crashesSeguidos = 0;
} reinicio;
static inline void migalha(Etapa e) { migalhas.etapa = (uint8_t)e; }

const char* nomeMotivo(esp_reset_reason_t m) {
  switch (m) {
    case ESP_RST_POWERON:   return "ligada";
    case ESP_RST_EXT:       return "botao reset";
    case ESP_RST_SW:        return "reinicio pedido";
    case ESP_RST_PANIC:     return "CRASH (panic)";
    case ESP_RST_INT_WDT:   return "CRASH (watchdog de interrupcao)";
    case ESP_RST_TASK_WDT:  return "CRASH (watchdog de tarefa)";
    case ESP_RST_WDT:       return "CRASH (watchdog)";
    case ESP_RST_BROWNOUT:  return "QUEDA DE TENSAO (brownout)";
    case ESP_RST_DEEPSLEEP: return "acordou do sono";
    default:                return "desconhecido";
  }
}
void analisarReinicio() {
  reinicio.motivo = esp_reset_reason();
  reinicio.foiCrash = reinicio.motivo == ESP_RST_PANIC || reinicio.motivo == ESP_RST_INT_WDT ||
                      reinicio.motivo == ESP_RST_TASK_WDT || reinicio.motivo == ESP_RST_WDT ||
                      reinicio.motivo == ESP_RST_BROWNOUT;
  reinicio.etapaCrash = Etapa::ARRANQUE;                          // sem dados válidos: "arranque"
  bool valida = migalhas.magia == MAGIA_RTC && migalhas.etapa < (uint8_t)Etapa::TOTAL;
  if (!valida) { migalhas.magia = MAGIA_RTC; migalhas.etapa = 0; migalhas.crashesSeguidos = 0; }
  if (reinicio.foiCrash && valida) {
    reinicio.etapaCrash = (Etapa)migalhas.etapa;
    migalhas.crashesSeguidos = min<uint8_t>(migalhas.crashesSeguidos + 1, 99);
  } else if (!reinicio.foiCrash) {
    migalhas.crashesSeguidos = 0;
  }
  reinicio.crashesSeguidos = migalhas.crashesSeguidos;
  migalha(Etapa::ARRANQUE);
}
void imprimirReinicio() {
  Serial.printf("[DIAG] ultimo reinicio: %s", nomeMotivo(reinicio.motivo));
  if (reinicio.foiCrash) Serial.printf(" durante '%s' (%u seguido(s))", NOMES_ETAPA[(uint8_t)reinicio.etapaCrash], (unsigned)reinicio.crashesSeguidos);
  Serial.printf(" | pilha livre min: %lu B\n", (unsigned long)diag.pilhaLivreMin);
}

// ---- Objetos de hardware -----------------------------------------------------
struct OrdemCor { const char* nome; neoPixelType tipo; };
const OrdemCor ORDENS_COR[] = {
  {"RGB", NEO_RGB}, {"GRB", NEO_GRB}, {"BRG", NEO_BRG}, {"RBG", NEO_RBG}, {"GBR", NEO_GBR}, {"BGR", NEO_BGR},
};
const uint8_t NUM_ORDENS_COR = sizeof(ORDENS_COR) / sizeof(ORDENS_COR[0]);
uint8_t ordemCor = ORDEM_COR_PADRAO;
Adafruit_NeoPixel matriz(NUM_LEDS, PINO_MATRIZ, NEO_RGB + NEO_KHZ800);   // tipo final aplicado no setup()
WiFiManager wm;
WebServer server(PORTA_SERVIDOR_WEB);

Preferences preferencias;
uint8_t brilhoGeralPct = BRILHO_GERAL_PADRAO;
Paleta paletaAtual = Paleta::MANGUE;
// Teste de cores: vermelho -> verde -> azul puros, 2 s cada (pedido na Web)
#define DURACAO_FASE_TESTE_MS 2000
unsigned long tInicioTesteCor = 0;
bool testeCorAtivo = false;
int8_t faseTesteCor(unsigned long agora) {       // -1 = sem teste, 0/1/2 = R/G/B
  if (!testeCorAtivo) return -1;
  unsigned long t = agora - tInicioTesteCor;
  if (t >= 3UL * DURACAO_FASE_TESTE_MS) { testeCorAtivo = false; return -1; }
  return (int8_t)(t / DURACAO_FASE_TESTE_MS);
}
uint8_t espelho[NUM_LEDS][3];            // cópia do último frame (ordem lógica x,y) p/ a página
float vista[NUM_LEDS][3];                // frame "como a pessoa o vê" (antes da rotação física)
uint32_t corUnica = COR_UNICA_PADRAO;    // 0xRRGGBB, usada na paleta COR_UNICA

// Cor (em estado SUCESSO) de uma emoção na paleta ativa
void corEmocao(Emocao e, float* rgb) {
  const Expressao& ex = EXPRESSOES[(uint8_t)e];
  switch (paletaAtual) {
    case Paleta::MANGUE:
      rgb[0] = ex.r; rgb[1] = ex.g; rgb[2] = ex.b; return;
    case Paleta::COR_UNICA:            // a emoção fica no rosto e na respiração
      rgb[0] = (corUnica >> 16) & 0xFF; rgb[1] = (corUnica >> 8) & 0xFF; rgb[2] = corUnica & 0xFF; return;
    default: {
      const uint8_t* c = CORES_PALETA[(uint8_t)paletaAtual][(uint8_t)e];
      rgb[0] = c[0]; rgb[1] = c[1]; rgb[2] = c[2]; return;
    }
  }
}

// Temporizadores de módulo
unsigned long tFrame = 0, tSolo = 0, tEnergia = 0, tRede = 0, tLog = 0;

/*
 * ============================================================================
 *  3. UTILITÁRIOS
 * ============================================================================
 */
static inline float limitar(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float misturar(float a, float b, float t) { return a + (b - a) * t; }
static inline float suaveStep(float t) { t = limitar(t, 0, 1); return t * t * (3 - 2 * t); }
static inline float aleatorio(float a, float b) { return a + (b - a) * (random(0, 10001) / 10000.0f); }
// Filtro exponencial independente do frame-rate (constante de tempo tau)
static inline float suavizarExp(float atual, float alvo, float dtMs, float tauMs) {
  return atual + (alvo - atual) * (1.0f - expf(-dtMs / tauMs));
}
// Comparação segura contra overflow do millis() (~49,7 dias)
static inline bool jaChegou(unsigned long agora, unsigned long instante) {
  return (long)(agora - instante) >= 0;
}
static inline bool temporizador(unsigned long& tAnterior, unsigned long intervalo, unsigned long agora) {
  if (agora - tAnterior < intervalo) return false;
  tAnterior = agora;
  return true;
}
static inline const Expressao& expressaoDe(Emocao e) { return EXPRESSOES[(uint8_t)e]; }

// Índice físico do LED, com rotação configurável
static inline uint16_t indicePixel(uint8_t x, uint8_t y) {
#if ROTACAO_MATRIZ == 1
  uint8_t t = x; x = LARGURA - 1 - y; y = t;
#elif ROTACAO_MATRIZ == 2
  x = LARGURA - 1 - x; y = ALTURA - 1 - y;
#elif ROTACAO_MATRIZ == 3
  uint8_t t = x; x = y; y = ALTURA - 1 - t;
#endif
  return y * LARGURA + x;
}

const char* nomeStatus(StatusHardware s) {
  return s == StatusHardware::SUCESSO ? "sucesso" : (s == StatusHardware::ERRO ? "erro" : "tentando");
}
const char* nomeRede(EstadoRede r) {
  switch (r) {
    case EstadoRede::PORTAL:  return "portal";
    case EstadoRede::LIGADO:  return "ligado";
    case EstadoRede::PERDIDO: return "perdido";
    default:                  return "a_ligar";
  }
}
const char* nomeModo(ModoEcra m) {
  switch (m) {
    case ModoEcra::ACORDAR:   return "acordar";
    case ModoEcra::ADORMECER: return "adormecer";
    case ModoEcra::PROTETOR:  return "protetor";
    default:                  return "rosto";
  }
}

// Interação = alguém está a dar atenção. Se houver um problema de Wi-Fi por
// resolver, é o momento certo para o relembrar (no máximo 1x por minuto).
void registarInteracao(unsigned long agora) {
  emo.tUltimaInteracao = agora;
  if (subs[(uint8_t)Sub::WIFI].saude == Saude::AVISO && !vis.avisoAtivo &&
      agora - vis.tUltimoAviso > RELEMBRAR_APOS_MS) vis.avisoPendente = true;
  // Erro persistente noutro componente: relembrar com o painel (máx. 1x/min)
  if (!painel.ativo && agora - painel.tUltimoAuto > RELEMBRAR_APOS_MS) {
    for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL; i++) {
      if (i == (uint8_t)Sub::WIFI) continue;
      if (subs[i].saude == Saude::ERRO) { abrirPainel(agora, DURACAO_PAINEL_ERRO_MS, i); painel.tUltimoAuto = agora; break; }
    }
  }
}

// Uma "pessoa" é algo perto que se mexe. Um objeto imóvel (parede, vaso)
// deixa de contar ao fim de 45 s — senão a planta nunca dormiria.
bool pessoaPresente(unsigned long agora) {
  return us.pessoaPerto && (agora - us.tUltimaVariacao < US_OBJETO_ESTATICO_MS);
}

// Estado agregado dos sensores obrigatórios (o ultrassom é opcional e não entra)
StatusHardware statusGeral() {
  if (imu.status == StatusHardware::ERRO || solo.status == StatusHardware::ERRO) return StatusHardware::ERRO;
  if (imu.status == StatusHardware::TENTANDO || solo.status == StatusHardware::TENTANDO) return StatusHardware::TENTANDO;
  return StatusHardware::SUCESSO;
}

/*
 * ============================================================================
 *  4. MÓDULO EMOÇÕES & ENERGIA
 * ============================================================================
 *  Prioridade (da maior para a menor):
 *   1. Transitória (SUSTO, RAIVA, ALEGRIA-alívio) — reflexos, expiram sozinhas
 *   2. Manual (Web)                                — expira (padrão 10 s)
 *   3. Base: SEDE > SONO (energia baixa) > AMOR > CURIOSO > ALEGRIA > ESPERA
 */
void definirTransitoria(Emocao e, uint32_t duracaoMs, unsigned long agora) {
  emo.transitoriaAtiva = true;
  emo.transitoria = e;
  emo.fimTransitoria = agora + duracaoMs;
}

void registarSusto(unsigned long agora) {
  if (agora - emo.tUltimoSusto < REFRATARIO_SUSTO_MS) return;   // um susto por evento
  emo.tUltimoSusto = agora;
  emo.energia = limitar(emo.energia - ENERGIA_CUSTO_SUSTO, 0, 100);
  registarInteracao(agora);

  if (agora - emo.inicioJanelaRaiva > JANELA_RAIVA_MS) { emo.inicioJanelaRaiva = agora; emo.sustosNaJanela = 0; }
  if (++emo.sustosNaJanela >= SUSTOS_PARA_RAIVA) {
    emo.sustosNaJanela = 0;
    definirTransitoria(Emocao::RAIVA, DURACAO_RAIVA_MS, agora);
    Serial.println("[EMO] Abanaram-me demasiado -> RAIVA");
  } else if (!(emo.transitoriaAtiva && emo.transitoria == Emocao::RAIVA)) {
    definirTransitoria(Emocao::SUSTO, DURACAO_SUSTO_MS, agora);
  }
}

Emocao calcularEmocaoBase(unsigned long agora) {
  if (solo.comSede) return Emocao::SEDE;
  if (emo.energia < ENERGIA_LIMIAR_SONO) return Emocao::SONO;
  if (pessoaPresente(agora)) {
    if (agora - us.tPessoaDesde > AMOR_APOS_MS && emo.energia > 50) return Emocao::AMOR;
    return Emocao::CURIOSO;
  }
  if (emo.tUltimaInteracao != 0 && agora - emo.tUltimaInteracao < INTERACAO_RECENTE_MS) return Emocao::ALEGRIA;
  return Emocao::ESPERA;
}

void aplicarEmocao(Emocao nova, unsigned long agora) {
  if (nova == emo.atual) return;
  vis.emocaoAnterior = emo.atual;
  vis.tInicioCrossfade = agora;
  emo.atual = nova;
  Serial.printf("[EMO] %s -> %s (energia %.0f)\n",
                expressaoDe(vis.emocaoAnterior).nome, expressaoDe(nova).nome, emo.energia);
}

void atualizarEmocao(unsigned long agora) {
  if (emo.transitoriaAtiva && jaChegou(agora, emo.fimTransitoria)) emo.transitoriaAtiva = false;
  if (emo.manualAtiva && jaChegou(agora, emo.fimManual)) emo.manualAtiva = false;

  Emocao alvo;
  if (emo.transitoriaAtiva)      alvo = emo.transitoria;
  else if (emo.manualAtiva)      alvo = emo.manual;
  else                           alvo = calcularEmocaoBase(agora);

  aplicarEmocao(alvo, agora);
}

// Energia: modelo de "metabolismo" simples, atualizado 1x por segundo
void atualizarEnergia(unsigned long agora) {
  if (!temporizador(tEnergia, INTERVALO_ENERGIA_MS, agora)) return;
  float delta = -ENERGIA_DECAI_MIN / 60.0f;
  if (emo.tUltimaInteracao != 0 && agora - emo.tUltimaInteracao < 2000) delta += ENERGIA_GANHO_INTER_S;
  if (solo.comSede) delta -= ENERGIA_DRENO_SEDE_MIN / 60.0f;
  if (vis.modo == ModoEcra::PROTETOR) delta += ENERGIA_REPOUSO_MIN / 60.0f;
  emo.energia = limitar(emo.energia + delta, 0, 100);
}

/*
 * ============================================================================
 *  4b. MÓDULO SAÚDE DO SISTEMA (sinalização de avarias — princípios de UX)
 * ============================================================================
 *  1. CANAIS SEPARADOS: o rosto (forma, cor da paleta, respiração) só mostra
 *     emoções. O sistema tem o seu próprio canal: um INDICADOR no canto da
 *     matriz + o painel "Saúde do sistema" na página Web.
 *     (Na V7.4 o erro pintava o rosto de vermelho: parecia "paleta avariada".)
 *  2. NÃO SÓ COR (WCAG 1.4.1): cada nível tem cor + padrão temporal:
 *       apagado = tudo bem | azul a pulsar = a iniciar
 *       âmbar a pulsar = atenção | vermelho FIXO = erro (+ olhos em X)
 *  3. GRAVIDADE CERTA: AUSENTE (sensor opcional não instalado) não é ERRO.
 *     Falsos alarmes ensinam a ignorar alarmes.
 *  4. DEBOUNCE: quedas de Wi-Fi < 5 s não alarmam.
 *  5. TECNOLOGIA CALMA: a planta reage (CONFUSO 4 s, ícone 1x) e acalma;
 *     o ícone só volta quando alguém se aproxima/mexe (máx. 1x/min).
 *  6. RECONHECER, DIAGNOSTICAR, RECUPERAR (Nielsen #9): a página diz qual
 *     componente, desde quando e o que fazer; e guarda um histórico — quando o
 *     Wi-Fi cai, a página não abre, mas depois mostra o que aconteceu.
 */
Nivel nivelDe(Saude s) {
  switch (s) {
    case Saude::ERRO:    return Nivel::ERRO;
    case Saude::AVISO:   return Nivel::AVISO;
    case Saude::A_LIGAR: return Nivel::INFO;
    default:             return Nivel::OK;      // OK, AUSENTE, DESATIVADO
  }
}

Nivel nivelGeral() {
  Nivel n = Nivel::OK;
  for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL; i++) n = max(n, nivelDe(subs[i].saude));
  return n;
}

void registarEvento(Sub sub, Saude de, Saude para, unsigned long agora) {
  eventos[iEvento] = {agora, (uint8_t)sub, de, para};
  iEvento = (iEvento + 1) % MAX_EVENTOS;
  if (nEventos < MAX_EVENTOS) nEventos++;
  Serial.printf("[SAUDE] %s: %s -> %s\n", NOMES_SUB[(uint8_t)sub], NOMES_SAUDE[(uint8_t)de], NOMES_SAUDE[(uint8_t)para]);
}

void avaliarSaude(unsigned long agora) {
  if (!temporizador(tSaude, INTERVALO_SAUDE_MS, agora)) return;
  Saude novo[(uint8_t)Sub::TOTAL];

  novo[(uint8_t)Sub::IMU] = imu.status == StatusHardware::ERRO ? Saude::ERRO
                          : imu.status == StatusHardware::TENTANDO ? Saude::A_LIGAR : Saude::OK;

  Saude wifiAntes = subs[(uint8_t)Sub::WIFI].saude;
  switch (rede.estado) {
    case EstadoRede::LIGADO:  novo[(uint8_t)Sub::WIFI] = Saude::OK; break;
    case EstadoRede::A_LIGAR: novo[(uint8_t)Sub::WIFI] = Saude::A_LIGAR; break;
    case EstadoRede::PORTAL:  novo[(uint8_t)Sub::WIFI] = Saude::AVISO; break;      // precisa de ação
    case EstadoRede::PERDIDO:                                                       // debounce
      novo[(uint8_t)Sub::WIFI] = (agora - rede.tPerda > WIFI_AVISO_APOS_MS) ? Saude::AVISO : wifiAntes;
      break;
  }
#if USAR_SENSOR_SOLO
  novo[(uint8_t)Sub::SOLO] = !solo.presente ? Saude::AUSENTE : (solo.temLeitura ? Saude::OK : Saude::A_LIGAR);
#else
  novo[(uint8_t)Sub::SOLO] = Saude::DESATIVADO;
#endif
#if USAR_ULTRASSOM
  novo[(uint8_t)Sub::ULTRASSOM] = us.detectado ? Saude::OK : (us.decidido ? Saude::AUSENTE : Saude::A_LIGAR);
#else
  novo[(uint8_t)Sub::ULTRASSOM] = Saude::DESATIVADO;
#endif

  for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL; i++) {
    if (novo[i] == subs[i].saude) continue;
    registarEvento((Sub)i, subs[i].saude, novo[i], agora);
    bool problemaNovo = nivelDe(novo[i]) >= Nivel::AVISO && nivelDe(subs[i].saude) < Nivel::AVISO;
    subs[i].saude = novo[i];
    subs[i].desde = agora;
    // Problema novo (exceto Wi-Fi, que tem ícone próprio): painel 1x com o culpado
    if (problemaNovo && i != (uint8_t)Sub::WIFI && !painel.arranque) {
      abrirPainel(agora, DURACAO_PAINEL_ERRO_MS, i);
      painel.tUltimoAuto = agora;
    }
  }

  // Reações da "personagem" — breves, uma vez por mudança de estado
  Saude wifiAgora = subs[(uint8_t)Sub::WIFI].saude;
  if (wifiAgora == Saude::AVISO && wifiAntes != Saude::AVISO) {
    if (rede.estado == EstadoRede::PERDIDO) definirTransitoria(Emocao::CONFUSO, DURACAO_REACAO_MS, agora);
    vis.avisoPendente = true;                                     // mostra o ícone UMA vez
  }
  if (wifiAgora == Saude::OK && wifiAntes == Saude::AVISO && rede.jaLigou)
    definirTransitoria(Emocao::ALEGRIA, DURACAO_ALIVIO_MS, agora);   // alívio
}

/*
 * ============================================================================
 *  5. MÓDULO SENSOR: QMI8658 (I2C, máquina de estados não-bloqueante)
 * ============================================================================
 *  Registos (datasheet QST QMI8658A/C; confirmados no driver RIOT-OS/ESPHome):
 *   WHO_AM_I 0x00 = 0x05 | CTRL1 0x02 | CTRL2 0x03 | CTRL7 0x08
 *   AX_L 0x35 .. AZ_H 0x3A | RESET 0x60 <- 0xB0
 */
namespace QMI {
  constexpr uint8_t REG_WHO_AM_I = 0x00, VAL_WHO_AM_I = 0x05;
  constexpr uint8_t REG_CTRL1 = 0x02, REG_CTRL2 = 0x03, REG_CTRL7 = 0x08;
  constexpr uint8_t REG_AX_L  = 0x35, REG_RESET = 0x60, CMD_RESET = 0xB0;
  constexpr uint8_t CTRL1_AUTO_INC = 0x40;                     // incremento automático de endereço
  constexpr uint8_t CTRL2_4G_125HZ = (0b001 << 4) | 0x06;      // ±4 g, ODR 125 Hz
  constexpr uint8_t CTRL7_ACC_EN   = 0x01;                     // só acelerómetro (poupa energia)
  constexpr float   LSB_POR_G      = 8192.0f;                  // 32768 / 4 g
}

bool i2cEscrever(uint8_t end, uint8_t reg, uint8_t valor) {
  Wire.beginTransmission(end);
  Wire.write(reg);
  Wire.write(valor);
  return Wire.endTransmission() == 0;
}

bool i2cLer(uint8_t end, uint8_t reg, uint8_t* buf, uint8_t n) {
  Wire.beginTransmission(end);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;          // repeated start
  if (Wire.requestFrom(end, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

void imuFalhou(unsigned long agora, const char* motivo) {
  imu.leiturasFalhadas++;
  if (++imu.falhasSeguidas >= FALHAS_PARA_ERRO && imu.status != StatusHardware::ERRO) {
    imu.status = StatusHardware::ERRO;
    Serial.printf("[IMU] ERRO: %s\n", motivo);
  }
  imu.fase = EstadoIMU::Fase::PROCURAR;
  imu.tProximaAcao = agora + imu.backoffMs;                    // backoff exponencial
  imu.backoffMs = min<uint32_t>(imu.backoffMs * 2, BACKOFF_MAXIMO_MS);
}

/*
 *  ROTAÇÃO AUTOMÁTICA — de que lado está o "chão"?
 *  Com a matriz de pé, a gravidade projetada no plano (gx, gy) aponta para o
 *  chão. φ = atan2(-gx, gy) é o ângulo a que o "baixo" do rosto deve rodar:
 *  0° = normal, 90° = chão à esquerda, 180° = de cabeça para baixo, 270° = direita.
 *  Regras de UX: só roda se φ estiver a <= 15° de um múltiplo de 90° (entre
 *  janelas mantém-se: histerese natural); a posição tem de durar 0,4 s; com a
 *  placa deitada (> 40° da vertical) não faz nada.
 */
static float difAngulo(float a, float b) {                        // diferença em [-180, 180)
  float d = fmodf(a - b + 540.0f, 360.0f) - 180.0f;
  return d;
}

/*
 *  MONTAGEM DO SENSOR — leva o vetor do referencial do chip para o da matriz:
 *    v_matriz = R(quarto·90°) · M · v_sensor     (M = inverter X se espelho)
 *  Uma rotação dá um erro CONSTANTE em todas as posições; um espelho dá um erro
 *  que ALTERNA de sentido. Por isso a calibração usa 2 posições.
 */
void corrigirMontagem(float& x, float& y) {
  if (eixos.espelho) x = -x;
  float t;
  switch (eixos.quarto & 3) {
    case 1: t = x; x = -y; y = t;  break;          // R(90°)
    case 2: x = -x; y = -y;        break;          // R(180°)
    case 3: t = x; x = y; y = -t;  break;          // R(270°)
    default: break;
  }
}
static float anguloCru() {                                        // φ do vetor sem correção
  float phi = atan2f(-imu.cruX, imu.cruY) * 180.0f / PI;
  return phi < 0 ? phi + 360.0f : phi;
}
// Placa de pé e parada? (condição para medir na calibração)
static const char* condicaoMedicao() {
  float p = sqrtf(imu.cruX * imu.cruX + imu.cruY * imu.cruY);
  float t = sqrtf(p * p + imu.cruZ * imu.cruZ);
  if (imu.status != StatusHardware::SUCESSO) return "acelerometro indisponivel";
  if (t < 0.3f || p / t < cosf(VERTICAL_MAX_GRAUS * PI / 180.0f)) return "a placa tem de estar de pe (vertical)";
  if (imu.agitacao > 0.10f) return "segure a placa parada";
  return nullptr;
}
void aplicarMontagem(uint8_t quarto, bool espelho, unsigned long agora) {
  eixos.quarto = quarto & 3;
  eixos.espelho = espelho;
  preferencias.putUChar("eixq", eixos.quarto);
  preferencias.putUChar("eixm", eixos.espelho ? 1 : 0);
  // Recalcular a gravidade corrigida já (sem esperar pelo filtro)
  float x = imu.cruX, y = imu.cruY; corrigirMontagem(x, y);
  imu.gravX = x; imu.gravY = y; imu.gravZ = imu.cruZ;
  imu.calibrado = false; imu.amostrasCal = 0;                     // novo repouso
  mov.inclX = mov.inclY = 0; mov.emMovimento = false;
  rot.candidato = -1;
  Serial.printf("[ROT] montagem do sensor: %d graus%s\n", eixos.quarto * 90, eixos.espelho ? " + espelho" : "");
  (void)agora;
}

/*
 *  GIRAR e FIXAR — manipulação direta ("rodar até ficar direito"), com o
 *  modelo mental do bloqueio de rotação dos telemóveis.
 *   girar(+1) = ↻ 90° no sentido horário (visto de frente); girar(-1) = ↺.
 *   - Automática: o rosto gira JÁ e a planta APRENDE (corrige a montagem do
 *     sensor em ±90°), por isso continua a girar sozinha e certa.
 *   - Fixa: muda a posição fixa do rosto.
 *   fixar(true) = cadeado: o rosto fica na posição ATUAL; fixar(false) = automática.
 */
void definirQuarto(uint8_t q, unsigned long agora);            // definida mais abaixo
void girarRosto(int sentido, unsigned long agora) {
  uint8_t novo = (uint8_t)((rot.quarto + 4 + sentido) & 3);
  if (rot.automatica) {
    // Somar ±90° à correção da montagem soma ±90° ao ângulo medido (com ou sem espelho)
    aplicarMontagem((uint8_t)((eixos.quarto + 4 + sentido) & 3), eixos.espelho, agora);
  } else {
    rot.quartoFixo = novo;
    preferencias.putUChar("rotfix", novo);
  }
  definirQuarto(novo, agora);
  registarInteracao(agora);
}
void fixarRotacao(bool fixo, unsigned long agora) {
  rot.automatica = !fixo;
  rot.candidato = -1;
  if (fixo) { rot.quartoFixo = rot.quarto; preferencias.putUChar("rotfix", rot.quartoFixo); }
  preferencias.putUChar("autorot", rot.automatica ? 1 : 0);
  mostrarIconeBreve(fixo ? ICONE_FIXO : ICONE_LIVRE, fixo ? 255 : 0, fixo ? 170 : 200, fixo ? 40 : 90, agora);
  Serial.printf("[ROT] %s\n", fixo ? "orientacao FIXA" : "orientacao automatica");
  registarInteracao(agora);
}
void definirQuarto(uint8_t q, unsigned long agora) {
  rot.anguloInicio = rot.anguloExibido;
  rot.anguloAlvo = rot.anguloInicio + difAngulo(q * 90.0f, rot.anguloInicio);   // caminho mais curto
  rot.tInicioAnim = agora;
  rot.animando = true;
  rot.quarto = q;
  // Nova posição de "repouso": recalibrar para o olhar e o sono funcionarem
  imu.calibrado = false;
  imu.amostrasCal = 0;
  mov.inclX = mov.inclY = 0;
  mov.emMovimento = false;
  Serial.printf("[ROT] rosto a %d graus\n", q * 90);
}
void atualizarOrientacao(unsigned long agora) {
  float gx = imu.gravX, gy = imu.gravY, gz = imu.gravZ;
  float plano = sqrtf(gx * gx + gy * gy), total = sqrtf(gx * gx + gy * gy + gz * gz);
  rot.vertical = total > 0.3f && plano / total >= cosf(VERTICAL_MAX_GRAUS * PI / 180.0f);
  if (!rot.vertical) { rot.candidato = -1; return; }
  float phi = atan2f(-gx, gy) * 180.0f / PI;
  if (phi < 0) phi += 360.0f;
  rot.anguloMedido = phi;
  if (!rot.automatica) { rot.candidato = -1; return; }
  uint8_t q = (uint8_t)(lroundf(phi / 90.0f) & 3);
  if (fabsf(difAngulo(phi, q * 90.0f)) > LIMIAR_ROTACAO_GRAUS || q == rot.quarto) { rot.candidato = -1; return; }
  if (rot.candidato != (int8_t)q) { rot.candidato = q; rot.tCandidato = agora; return; }
  if (agora - rot.tCandidato >= ROTACAO_ESTAVEL_MS) {
    rot.candidato = -1;
    definirQuarto(q, agora);
    registarInteracao(agora);
  }
}

void processarAceleracao(float ax, float ay, float az, float dtMs, unsigned long agora) {
#if EIXO_TROCAR_XY
  float t = ax; ax = ay; ay = t;
#endif
#if EIXO_INVERTER_X
  ax = -ax;
#endif
#if EIXO_INVERTER_Y
  ay = -ay;
#endif
  // Vetor cru filtrado (antes da correção de montagem) — usado na calibração
  float k = 1.0f - expf(-dtMs / TAU_GRAVIDADE_MS);
  imu.cruX += (ax - imu.cruX) * k;
  imu.cruY += (ay - imu.cruY) * k;
  imu.cruZ += (az - imu.cruZ) * k;
  corrigirMontagem(ax, ay);
  // Separar gravidade (passa-baixo) de movimento brusco (passa-alto)
  imu.gravX += (ax - imu.gravX) * k;
  imu.gravY += (ay - imu.gravY) * k;
  imu.gravZ += (az - imu.gravZ) * k;
  float dx = ax - imu.gravX, dy = ay - imu.gravY, dz = az - imu.gravZ;
  float dinamica = sqrtf(dx * dx + dy * dy + dz * dz);
  imu.agitacao = max(dinamica, imu.agitacao * expf(-dtMs / 300.0f));
  atualizarOrientacao(agora);

  // Calibração automática da posição de repouso (funciona com a placa
  // deitada OU de pé, como um rosto na parede)
  if (!imu.calibrado) {
    if (imu.agitacao < 0.05f) {
      if (++imu.amostrasCal >= AMOSTRAS_CALIBRACAO) {
        imu.refX = imu.gravX; imu.refY = imu.gravY; imu.refZ = imu.gravZ;
        imu.calibrado = true;
        Serial.printf("[IMU] Repouso calibrado: (%.2f, %.2f, %.2f) g\n", imu.refX, imu.refY, imu.refZ);
      }
    } else {
      imu.amostrasCal = 0;
    }
    return;
  }

  mov.inclX = limitar(imu.gravX - imu.refX, -1, 1);
  mov.inclY = limitar(imu.gravY - imu.refY, -1, 1);
  float incl = sqrtf(mov.inclX * mov.inclX + mov.inclY * mov.inclY);
  if (!mov.emMovimento && incl > INCLINACAO_ENTRA) mov.emMovimento = true;      // histerese:
  else if (mov.emMovimento && incl < INCLINACAO_SAI) mov.emMovimento = false;   // evita cintilação

  if (mov.emMovimento || dinamica > LIMIAR_INTERACAO_G) registarInteracao(agora);
  if (dinamica > LIMIAR_SUSTO_G) registarSusto(agora);
}

void atualizarIMU(unsigned long agora) {
  switch (imu.fase) {
    case EstadoIMU::Fase::PROCURAR: {
      if (!jaChegou(agora, imu.tProximaAcao)) return;
      const uint8_t candidatos[2] = {QMI_END_PRIMARIO, QMI_END_ALTERNATIVO};
      uint8_t encontrado = 0, id = 0;
      for (uint8_t i = 0; i < 2 && !encontrado; i++) {
        if (i2cLer(candidatos[i], QMI::REG_WHO_AM_I, &id, 1) && id == QMI::VAL_WHO_AM_I) encontrado = candidatos[i];
      }
      if (!encontrado) { imuFalhou(agora, "QMI8658 nao responde (WHO_AM_I)"); return; }
      if (!i2cEscrever(encontrado, QMI::REG_RESET, QMI::CMD_RESET)) { imuFalhou(agora, "reset falhou"); return; }
      imu.endereco = encontrado;
      imu.fase = EstadoIMU::Fase::AGUARDAR_RESET;
      imu.tProximaAcao = agora + ESPERA_RESET_QMI_MS;            // espera SEM bloquear
      return;
    }
    case EstadoIMU::Fase::AGUARDAR_RESET: {
      if (!jaChegou(agora, imu.tProximaAcao)) return;
      uint8_t verificacao = 0;
      bool ok = i2cEscrever(imu.endereco, QMI::REG_CTRL1, QMI::CTRL1_AUTO_INC)
             && i2cEscrever(imu.endereco, QMI::REG_CTRL2, QMI::CTRL2_4G_125HZ)
             && i2cEscrever(imu.endereco, QMI::REG_CTRL7, QMI::CTRL7_ACC_EN)
             && i2cLer(imu.endereco, QMI::REG_CTRL2, &verificacao, 1)
             && verificacao == QMI::CTRL2_4G_125HZ;              // lê de volta: confirma
      if (!ok) { imuFalhou(agora, "configuracao nao confirmada"); return; }
      imu.fase = EstadoIMU::Fase::ATIVO;
      imu.tUltimaLeitura = agora;
      Serial.printf("[IMU] QMI8658 ativo em 0x%02X\n", imu.endereco);
      return;
    }
    case EstadoIMU::Fase::ATIVO: {
      if (agora - imu.tUltimaLeitura < INTERVALO_IMU_MS) return;
      float dtMs = (float)(agora - imu.tUltimaLeitura);
      imu.tUltimaLeitura = agora;
      uint8_t b[6];
      if (!i2cLer(imu.endereco, QMI::REG_AX_L, b, 6)) {
        imu.leiturasFalhadas++;
        if (++imu.falhasSeguidas >= FALHAS_PARA_ERRO) imuFalhou(agora, "leituras consecutivas falhadas");
        return;
      }
      int16_t rx = (int16_t)((b[1] << 8) | b[0]);
      int16_t ry = (int16_t)((b[3] << 8) | b[2]);
      int16_t rz = (int16_t)((b[5] << 8) | b[4]);
      if (rx == 0 && ry == 0 && rz == 0) return;                 // 1ª amostra ainda não pronta
      imu.falhasSeguidas = 0;
      imu.backoffMs = BACKOFF_INICIAL_MS;
      imu.leiturasOk++;
      if (imu.status != StatusHardware::SUCESSO) {
        imu.status = StatusHardware::SUCESSO;
        Serial.println("[IMU] Leitura OK");
      }
      processarAceleracao(rx / QMI::LSB_POR_G, ry / QMI::LSB_POR_G, rz / QMI::LSB_POR_G, dtMs, agora);
      return;
    }
  }
}

/*
 * ============================================================================
 *  6. MÓDULO SENSOR: HC-SR04 (interrupção no ECHO, sem pulseIn())
 * ============================================================================
 *  pulseIn() bloqueia até 1 s por defeito. Aqui o ECHO gera interrupções
 *  CHANGE: subida guarda micros(), descida calcula a duração.
 *  Deteção automática: sem nenhuma resposta em 10 disparos -> "ausente"
 *  (com INPUT_PULLDOWN, um pino desligado fica em LOW e não gera flancos).
 */
// A ISR não pode depender de código em flash (a escrita na NVS desliga a
// cache): lê o registo de entrada do GPIO diretamente quando disponível.
#if defined(GPIO_IN_REG) && defined(REG_READ)
#define LER_ECHO() ((REG_READ(GPIO_IN_REG) >> PINO_US_ECHO) & 1)
#else
#define LER_ECHO() digitalRead(PINO_US_ECHO)
#endif
void IRAM_ATTR isrEco() {
  unsigned long t = micros();
  if (LER_ECHO()) { ecoInicioUs = t; ecoViuSubida = true; }
  else if (ecoViuSubida)         { ecoDuracaoUs = t - ecoInicioUs; ecoPronto = true; }
}

static float mediana3(float a, float b, float c) {
  return max(min(a, b), min(max(a, b), c));
}

// Separado da ISR para poder ser testado/injetado
void ultrassomProcessarDuracao(unsigned long duracaoUs, unsigned long agora) {
  // 58 µs/cm ≈ ida e volta a 343 m/s (20 °C). Com DHT: v = 331,3 + 0,606·T
  float cm = (duracaoUs > 25000UL) ? 999.0f : duracaoUs / 58.0f;
  us.janela[us.idx] = cm;
  us.idx = (us.idx + 1) % 3;
  us.distCm = mediana3(us.janela[0], us.janela[1], us.janela[2]);
  if (fabsf(us.distCm - us.distUltima) > US_VARIACAO_MIN_CM) us.tUltimaVariacao = agora;
  us.distUltima = us.distCm;

  bool antes = us.pessoaPerto;
  if (!us.pessoaPerto && us.distCm < US_PERTO_ENTRA_CM) us.pessoaPerto = true;
  else if (us.pessoaPerto && us.distCm > US_PERTO_SAI_CM) us.pessoaPerto = false;
  if (us.pessoaPerto && !antes) { us.tPessoaDesde = agora; registarInteracao(agora); }

  // Velocidade de aproximação numa janela de ~300 ms
  if (agora - us.tRef >= 300) {
    float delta = us.distRef - us.distCm;                       // >0 = aproximou-se
    if (delta > 25.0f && us.distCm < 30.0f) registarSusto(agora);   // aproximação brusca
    if (fabsf(delta) > 8.0f && min(us.distCm, us.distRef) < 150.0f) registarInteracao(agora);
    us.distRef = us.distCm;
    us.tRef = agora;
  }
}

void atualizarUltrassom(unsigned long agora) {
#if USAR_ULTRASSOM
  if (us.aguardarEco) {
    if (ecoPronto) {
      unsigned long dur = ecoDuracaoUs;
      ecoPronto = false;
      us.aguardarEco = false;
      us.semResposta = 0;
      if (!us.detectado) { us.detectado = true; Serial.println("[US] HC-SR04 detetado"); }
      us.decidido = true;
      ultrassomProcessarDuracao(dur, agora);
    } else if (agora - us.tDisparo > US_TIMEOUT_MS) {
      us.aguardarEco = false;
      if (us.semResposta + 1 >= US_FALHAS_AUSENTE) us.decidido = true;
      if (++us.semResposta >= US_FALHAS_AUSENTE && us.detectado) {
        us.detectado = false;
        us.pessoaPerto = false;
        us.distCm = 999;
        Serial.println("[US] HC-SR04 sem resposta -> considerado desligado");
      }
    }
    return;
  }
  // Ao ligar, sonda depressa até decidir (~1,3 s); ausente: sonda devagar
  unsigned long intervalo = (us.detectado || !us.decidido) ? US_INTERVALO_MS : US_INTERVALO_SONDA_MS;
  if (agora - us.tDisparo < intervalo) return;
  ecoViuSubida = false;
  ecoPronto = false;
  digitalWrite(PINO_US_TRIG, HIGH);
  delayMicroseconds(10);                 // pulso de disparo exigido pelo sensor
  digitalWrite(PINO_US_TRIG, LOW);
  us.tDisparo = agora;
  us.aguardarEco = true;
#else
  (void)agora;
#endif
}

/*
 * ============================================================================
 *  7. MÓDULO SENSOR: HUMIDADE DO SOLO (ADC1, sobreamostragem + histerese)
 * ============================================================================
 */
void atualizarSolo(unsigned long agora) {
#if USAR_SENSOR_SOLO
  if (!temporizador(tSolo, INTERVALO_SOLO_MS, agora)) return;
  uint32_t soma = 0;
  for (uint8_t i = 0; i < SOLO_AMOSTRAS; i++) soma += analogReadMilliVolts(PINO_SENSOR_SOLO);
  float mv = soma / (float)SOLO_AMOSTRAS;

  // Leitura saturada repetidamente = sensor solto/desligado.
  // Um sensor OPCIONAL ausente não é um "erro": não pinta o rosto de vermelho
  // (na V7.4 isto tapava a paleta). Fica "sem sensor" e a sede é desligada.
  solo.leiturasForaGama = (mv < 50 || mv > 3150) ? min<uint8_t>(solo.leiturasForaGama + 1, 10) : 0;
  if (solo.leiturasForaGama >= 3) {
    if (solo.presente) Serial.println("[SOLO] leitura fora de gama -> sensor considerado desligado");
    solo.presente = false;
    solo.status = StatusHardware::SUCESSO;
    solo.comSede = false;
    solo.percentagem = -1;
    return;
  }
  if (!solo.presente) Serial.println("[SOLO] sensor detetado");
  solo.presente = true;
  solo.mvFiltrado = solo.temLeitura ? solo.mvFiltrado + (mv - solo.mvFiltrado) * 0.3f : mv;
  solo.temLeitura = true;
  solo.status = StatusHardware::SUCESSO;
  solo.percentagem = (int)limitar((SOLO_MV_SECO - solo.mvFiltrado) * 100.0f / (SOLO_MV_SECO - SOLO_MV_MOLHADO), 0, 100);
  if (!solo.comSede && solo.percentagem < SOLO_PCT_ENTRA_SEDE) solo.comSede = true;
  else if (solo.comSede && solo.percentagem > SOLO_PCT_SAI_SEDE) solo.comSede = false;
#else
  (void)agora;
  solo.status = StatusHardware::SUCESSO;
#endif
}

/*
 * ============================================================================
 *  8. MOTOR VISUAL — CAMADA BASE: RESPIRAÇÃO (tamanho) + COR PERCEPTUAL
 * ============================================================================
 *  PORQUÊ (medido no banco de testes): o "coração" por brilho da V7.0 variava
 *  a luminância ~389% e saltava até 410% entre frames -> parecia a piscar.
 *  Um ser vivo calmo mexe-se devagar: aqui o rosto ENCOLHE e CRESCE com uma
 *  onda cossenoidal (sem arestas), e o brilho fica quase fixo.
 *  - Fase acumulada: mudar o ritmo não provoca saltos.
 *  - Ritmo e amplitude seguem a emoção com filtros lentos (3 s / 2 s).
 */
struct Respiracao { float escala; float fatorBrilho; };

Respiracao atualizarRespiracao(const Expressao& ex, StatusHardware st, float dtMs) {
  float energia01 = emo.energia / 100.0f;
  float ritmoAlvo = ex.respMin * (0.8f + 0.4f * energia01);       // pouca energia = mais lento
  float amplAlvo  = ex.amplitude * (0.7f + 0.3f * energia01);
  if (st == StatusHardware::ERRO) { ritmoAlvo = 26; amplAlvo = 0.10f; }   // "ofegante"
  vis.ritmo     = suavizarExp(vis.ritmo, ritmoAlvo, dtMs, TAU_RITMO_MS);
  vis.amplitude = suavizarExp(vis.amplitude, amplAlvo, dtMs, TAU_AMPLITUDE_MS);
  vis.fase += dtMs * vis.ritmo / 60000.0f;
  vis.fase -= floorf(vis.fase);

  float onda = 0.5f - 0.5f * cosf(2.0f * PI * vis.fase);          // 0..1, derivada contínua
  Respiracao r;
  r.escala = 1.0f - vis.amplitude * (1.0f - onda);                 // inspira = tamanho cheio
  float intensidade = ex.intensidade * (0.75f + 0.25f * energia01);
  r.fatorBrilho = BRILHO_BASE * intensidade * (1.0f - MODULACAO_BRILHO + MODULACAO_BRILHO * 2.0f * onda);
  return r;
}

// Reamostragem por ÁREA (box filter): cada LED recebe a média da porção do
// rosto que lhe cabe depois de encolhido à volta do centro. É isto que
// permite "tamanhos" intermédios numa matriz de 8x8 sem saltar de pixel.
void escalarRosto(const float* origem, float* destino, float escala) {
  if (escala >= 0.999f) { memcpy(destino, origem, sizeof(float) * NUM_LEDS); return; }
  const float c = LARGURA / 2.0f, inv = 1.0f / escala;
  const float ganho = powf(inv, 2.0f * COMPENSAR_AREA);             // área encolhe com s²
  for (int y = 0; y < ALTURA; y++) {
    float v0 = (y - c) * inv + c, v1 = (y + 1 - c) * inv + c;
    for (int x = 0; x < LARGURA; x++) {
      float u0 = (x - c) * inv + c, u1 = (x + 1 - c) * inv + c;
      float soma = 0;
      for (int sy = max(0, (int)floorf(v0)); sy < min(ALTURA, (int)ceilf(v1)); sy++) {
        float oy = min(v1, (float)sy + 1) - max(v0, (float)sy);
        if (oy <= 0) continue;
        for (int sx = max(0, (int)floorf(u0)); sx < min(LARGURA, (int)ceilf(u1)); sx++) {
          float ox = min(u1, (float)sx + 1) - max(u0, (float)sx);
          if (ox > 0) soma += origem[sy * LARGURA + sx] * ox * oy;
        }
      }
      destino[y * LARGURA + x] = min(1.0f, ganho * soma / ((u1 - u0) * (v1 - v0)));
    }
  }
}

// ---- Cor perceptual: OKLab / OKLCh (Björn Ottosson, 2020) -------------------
// Interpolar em RGB faz verde->roxo passar por cinzento escuro. Em OKLCh a
// luminosidade (L) e a saturação (C) mudam de forma independente e o matiz (h)
// roda pelo caminho mais curto: a cor "desliza" sem escurecer.
// Os valores dos LED (PWM) são tratados como RGB linear 0..1.
static void rgbParaOklch(const float* rgb, float* lch) {
  float r = rgb[0] / 255.0f, g = rgb[1] / 255.0f, b = rgb[2] / 255.0f;
  float l = cbrtf(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
  float m = cbrtf(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
  float s = cbrtf(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);
  float L = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
  float A = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
  float B = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
  lch[0] = L; lch[1] = sqrtf(A * A + B * B); lch[2] = atan2f(B, A);
}
static void oklchParaRgb(const float* lch, float* rgb) {
  float A = lch[1] * cosf(lch[2]), B = lch[1] * sinf(lch[2]);
  float l = lch[0] + 0.3963377774f * A + 0.2158037573f * B;
  float m = lch[0] - 0.1055613458f * A - 0.0638541728f * B;
  float s = lch[0] - 0.0894841775f * A - 1.2914855480f * B;
  l = l * l * l; m = m * m * m; s = s * s * s;
  rgb[0] = 255.0f * limitar( 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s, 0, 1);
  rgb[1] = 255.0f * limitar(-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s, 0, 1);
  rgb[2] = 255.0f * limitar(-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s, 0, 1);
}
void suavizarCor(const float* rgbAlvo, float dtMs, float* rgbSaida) {
  float alvo[3];
  rgbParaOklch(rgbAlvo, alvo);
  float k = 1.0f - expf(-dtMs / TAU_COR_MS);
  vis.corLch[0] += (alvo[0] - vis.corLch[0]) * k;
  vis.corLch[1] += (alvo[1] - vis.corLch[1]) * k;
  if (alvo[1] > 0.02f) {                                           // cor sem saturação não tem matiz
    float dh = alvo[2] - vis.corLch[2];
    dh = atan2f(sinf(dh), cosf(dh));                                // caminho mais curto no círculo
    vis.corLch[2] += dh * k;
  }
  oklchParaRgb(vis.corLch, rgbSaida);
}

/*
 * ============================================================================
 *  9. MOTOR VISUAL — CAMADA INTERMÉDIA: ROSTO PROCEDURAL
 * ============================================================================
 */
// Pupilas: mola (puxa para o alvo) + amortecedor + "gravidade" da inclinação
void atualizarOlhos(float dtMs, unsigned long agora) {
  float dt = dtMs / 1000.0f;

  // 1. Escolher o alvo do olhar
  bool pessoa = pessoaPresente(agora);
  if (pessoa) {
    olhos.alvoX = 0; olhos.alvoY = 0;                            // olha para a pessoa
    olhos.tProximaSacada = agora + 1500;
  } else if (jaChegou(agora, olhos.tProximaSacada)) {
    bool voltarCentro = random(0, 3) == 0;                       // sacada: olhar à volta
    olhos.alvoX = voltarCentro ? 0 : aleatorio(-0.6f, 0.6f);   // olhar calmo, sem extremos
    olhos.alvoY = voltarCentro ? 0 : aleatorio(-0.4f, 0.4f);
    olhos.tProximaSacada = agora + (unsigned long)aleatorio(3000, 7000);
  }
  float alvoConv = (pessoa && us.distCm < US_VESGO_CM) ? 0.7f : 0.0f;
  olhos.convergencia = suavizarExp(olhos.convergencia, alvoConv, dtMs, 200);

  // 2. Integrar a física (Euler semi-implícito, estável para este dt)
  // A inclinação vem no referencial da matriz; o rosto pode estar rodado:
  // levar o vetor para o referencial do rosto (rotação de -φ).
  float fr = rot.anguloExibido * PI / 180.0f, cr = cosf(fr), sr = sinf(fr);
  float incRostoX =  cr * mov.inclX + sr * mov.inclY;
  float incRostoY = -sr * mov.inclX + cr * mov.inclY;
  float ax = PUPILA_RIGIDEZ * (olhos.alvoX - olhos.px) - PUPILA_AMORTECIMENTO * olhos.vx + PUPILA_GRAVIDADE * incRostoX;
  float ay = PUPILA_RIGIDEZ * (olhos.alvoY - olhos.py) - PUPILA_AMORTECIMENTO * olhos.vy + PUPILA_GRAVIDADE * incRostoY;
  olhos.vx += ax * dt;  olhos.vy += ay * dt;
  olhos.px += olhos.vx * dt;  olhos.py += olhos.vy * dt;
  // Bater na "órbita" do olho com ressalto
  if (olhos.px < -1) { olhos.px = -1; olhos.vx *= -0.4f; }
  if (olhos.px >  1) { olhos.px =  1; olhos.vx *= -0.4f; }
  if (olhos.py < -1) { olhos.py = -1; olhos.vy *= -0.4f; }
  if (olhos.py >  1) { olhos.py =  1; olhos.vy *= -0.4f; }

  // 3. Piscar — desligado por defeito (PISCAR_OLHOS). Se ligado: ocasional e lento.
#if PISCAR_OLHOS
  if (!olhos.aPiscar && jaChegou(agora, olhos.tProximoPiscar)) {
    olhos.aPiscar = true;
    olhos.tInicioPiscar = agora;
    olhos.duracaoPiscar = emo.energia < 30 ? 400 : DURACAO_PISCAR_MS;
  }
  if (olhos.aPiscar && agora - olhos.tInicioPiscar >= olhos.duracaoPiscar) {
    olhos.aPiscar = false;
    olhos.tProximoPiscar = agora + (unsigned long)aleatorio(4000, 9000);
  }
#else
  olhos.aPiscar = false;
#endif
}

float palpebraAtual(unsigned long agora) {
  float lid = 0;
  if (olhos.aPiscar) lid = sinf(PI * (agora - olhos.tInicioPiscar) / (float)olhos.duracaoPiscar);
  unsigned long t = agora - vis.tModo;
  if (vis.modo == ModoEcra::ACORDAR)   lid = max(lid, 1.0f - suaveStep(t / (float)DURACAO_ACORDAR_MS));
  if (vis.modo == ModoEcra::ADORMECER) lid = max(lid, suaveStep(t / (float)DURACAO_ADORMECER_MS));
  if (vis.modo == ModoEcra::PROTETOR)  lid = 1;
  return limitar(lid, 0, 1);
}

void desenharOlho(float* buf, FormaOlho forma, bool direito, float pupX, float pupY, float lid) {
  const DefOlho& d = DEF_OLHOS[(uint8_t)forma];
  const int x0 = direito ? LARGURA - 3 : 0;
  // Centro da pupila em coordenadas locais (o olho direito é espelhado)
  const float cx = 1.0f + (direito ? -pupX : pupX);
  const float cy = 1.0f + pupY;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {
      if (!(d.linhas[r] & (0b100 >> c))) continue;
      float v = 1.0f;
      if (d.temPupila) {                                          // pupila escura anti-aliased
        float w = max(0.0f, 1 - fabsf(c - cx)) * max(0.0f, 1 - fabsf(r - cy));
        v *= 1.0f - w;
      }
      if (d.pestaneja) v *= 1.0f - limitar(lid * 3.0f - r, 0, 1); // pálpebra desce de cima
      else if (lid > 0.5f && r < 2) v = 0;                        // formas sem pupila: fecham de vez
      int col = direito ? x0 + (2 - c) : x0 + c;
      float& p = buf[(LINHA_OLHOS + r) * LARGURA + col];
      p = max(p, v);
    }
  }
}

void desenharLinhaBits(float* buf, uint8_t linha, uint8_t bits) {
  for (int c = 0; c < LARGURA; c++)
    if (bits & (0x80 >> c)) buf[linha * LARGURA + c] = 1.0f;
}

void desenharRosto(float* buf, const Expressao& ex, bool olhosX, unsigned long agora) {
  float lid = palpebraAtual(agora);
  FormaOlho oe = olhosX ? FormaOlho::X : ex.olhoEsq;
  FormaOlho od = olhosX ? FormaOlho::X : ex.olhoDir;
  // Olhar ativo (inclinação ou pessoa à frente): olhos "^ ^" não têm pupila e
  // esconderiam o movimento do olhar -> abrem enquanto durar o estímulo.
  if (mov.emMovimento || pessoaPresente(agora)) {
    if (oe == FormaOlho::FELIZ) oe = FormaOlho::NORMAL;
    if (od == FormaOlho::FELIZ) od = FormaOlho::NORMAL;
  }
  // pupila em unidades de pixel; convergência: esquerdo -> direita, direito -> esquerda
  desenharOlho(buf, oe, false, olhos.px + olhos.convergencia, olhos.py, lid);
  desenharOlho(buf, od, true,  olhos.px - olhos.convergencia, olhos.py, lid);
  desenharLinhaBits(buf, LINHA_SOBRANCELHAS, ex.sobrancelhas);
  const uint8_t* boca = DEF_BOCAS[(uint8_t)(olhosX ? FormaBoca::TRISTE : ex.boca)];
  for (uint8_t i = 0; i < 3; i++) desenharLinhaBits(buf, LINHA_BOCA + i, boca[i]);
}

/*
 * ============================================================================
 *  10. MOTOR VISUAL — PROTETORES DE ECRÃ
 * ============================================================================
 */
static void paletaMare(float v, float* rgb) {
  // azul profundo -> verde-água -> verde mangue -> espuma
  static const float P[4][3] = {{0, 10, 45}, {0, 120, 110}, {40, 180, 40}, {170, 220, 150}};
  v = limitar(v, 0, 0.999f) * 3.0f;
  int i = (int)v; float t = v - i;
  for (int k = 0; k < 3; k++) rgb[k] = misturar(P[i][k], P[i + 1][k], t);
}

void protetorMare(unsigned long agora) {
  float t = agora / 1600.0f;                                      // maré lenta
  float cx = 3.5f + 2.5f * sinf(t * 0.31f), cy = 3.5f + 2.5f * cosf(t * 0.23f);
  for (int y = 0; y < ALTURA; y++)
    for (int x = 0; x < LARGURA; x++) {
      float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
      float v = sinf(x * 0.55f + t * 0.9f) + sinf(y * 0.45f - t * 1.2f)
              + sinf((x + y) * 0.35f + t * 0.6f) + sinf(d * 0.9f - t * 1.6f);
      v = (v + 4.0f) / 8.0f;
      paletaMare(v * v * (3 - 2 * v), prot.rgb[y * LARGURA + x]);   // contraste suave
    }
}

static uint32_t assinaturaVida() {
  uint32_t h = 2166136261u;                                       // FNV-1a
  for (int i = 0; i < NUM_LEDS; i++) { h ^= prot.vida[i]; h *= 16777619u; }
  return h;
}
static void semearVida() {
  for (int i = 0; i < NUM_LEDS; i++) { prot.vida[i] = random(0, 100) < 35; prot.idadeVida[i] = 0; }
  prot.geracoes = 0;
  prot.assinaturas[0] = prot.assinaturas[1] = 0;
}

void protetorVida(unsigned long agora, float dtMs) {
  if (agora - prot.tPassoVida >= 700) {                           // 1 geração / 700 ms (calmo)
    prot.tPassoVida = agora;
    uint8_t prox[NUM_LEDS];
    int vivos = 0;
    for (int y = 0; y < ALTURA; y++)
      for (int x = 0; x < LARGURA; x++) {
        int n = 0;
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) n += prot.vida[((y + dy + ALTURA) % ALTURA) * LARGURA + (x + dx + LARGURA) % LARGURA];
        int i = y * LARGURA + x;
        prox[i] = prot.vida[i] ? (n == 2 || n == 3) : (n == 3);   // regras B3/S23 (toro)
        prot.idadeVida[i] = prox[i] ? min<uint16_t>(prot.idadeVida[i] + 1, 60) : 0;
        vivos += prox[i];
      }
    memcpy(prot.vida, prox, sizeof(prox));
    uint32_t a = assinaturaVida();
    bool estagnou = (a == prot.assinaturas[0] || a == prot.assinaturas[1]);   // estável ou período 2
    prot.assinaturas[1] = prot.assinaturas[0];
    prot.assinaturas[0] = a;
    if (vivos == 0 || estagnou || ++prot.geracoes > 150) semearVida();
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    prot.brilhoVida[i] = suavizarExp(prot.brilhoVida[i], prot.vida[i] ? 1.0f : 0.0f, dtMs, 450);
    float idade = prot.idadeVida[i] / 60.0f;                      // jovem = verde, velha = âmbar
    float b = prot.brilhoVida[i];
    prot.rgb[i][0] = b * misturar(30, 220, idade);
    prot.rgb[i][1] = b * misturar(210, 120, idade);
    prot.rgb[i][2] = b * misturar(70, 10, idade);
  }
}

static void semearVagalumes() {
  for (auto& v : prot.vag) {
    v.ax = aleatorio(0.15f, 0.45f); v.ay = aleatorio(0.12f, 0.40f);
    v.fx = aleatorio(0, 6.28f);     v.fy = aleatorio(0, 6.28f);
    v.fase = aleatorio(0, 6.28f);
    v.freqPisca = aleatorio(0.25f, 0.5f); v.fasePisca = aleatorio(0, 6.28f);
  }
}

void protetorVagalumes(unsigned long agora) {
  float t = agora / 1500.0f;                                      // deriva lenta
  for (int i = 0; i < NUM_LEDS; i++)                              // rasto: o ecrã "esquece" devagar
    for (int k = 0; k < 3; k++) prot.rgb[i][k] *= 0.94f;
  for (int i = 0; i < NUM_LEDS; i++) { prot.rgb[i][1] = max(prot.rgb[i][1], 6.0f); prot.rgb[i][2] = max(prot.rgb[i][2], 3.0f); }
  for (auto& v : prot.vag) {
    float x = 3.5f + 3.4f * sinf(t * v.ax + v.fx) * cosf(t * 0.13f + v.fase);
    float y = 3.5f + 3.4f * sinf(t * v.ay + v.fy);
    float brilho = 0.5f - 0.5f * cosf(t * v.freqPisca * 3.0f + v.fasePisca);   // brilho suave
    brilho *= brilho;
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    const float pesos[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
    const int dx[4] = {0, 1, 0, 1}, dy[4] = {0, 0, 1, 1};
    for (int k = 0; k < 4; k++) {
      int xx = x0 + dx[k], yy = y0 + dy[k];
      if (xx < 0 || xx >= LARGURA || yy < 0 || yy >= ALTURA) continue;
      float w = pesos[k] * brilho;
      float* p = prot.rgb[yy * LARGURA + xx];
      p[0] = max(p[0], 200 * w); p[1] = max(p[1], 255 * w); p[2] = max(p[2], 40 * w);
    }
  }
}

void iniciarProtetor(TipoProtetor tipo, unsigned long agora) {
  prot.tipo = tipo;
  prot.tInicio = agora;
  memset(prot.rgb, 0, sizeof(prot.rgb));
  memset(prot.brilhoVida, 0, sizeof(prot.brilhoVida));
  if (tipo == TipoProtetor::VIDA) semearVida();
  if (tipo == TipoProtetor::VAGALUMES) semearVagalumes();
}

// Devolve o "alfa de ciclo" (fade entre protetores)
float renderizarProtetor(unsigned long agora, float dtMs) {
  if (agora - prot.tInicio >= DURACAO_CADA_PROT_MS)
    iniciarProtetor((TipoProtetor)(((uint8_t)prot.tipo + 1) % (uint8_t)TipoProtetor::TOTAL), agora);
  switch (prot.tipo) {
    case TipoProtetor::MARE:      protetorMare(agora); break;
    case TipoProtetor::VIDA:      protetorVida(agora, dtMs); break;
    case TipoProtetor::VAGALUMES: protetorVagalumes(agora); break;
    default: break;
  }
  unsigned long t = agora - prot.tInicio;
  return min(suaveStep(t / 1000.0f), suaveStep((DURACAO_CADA_PROT_MS - t) / 1000.0f));
}

/*
 * ============================================================================
 *  11. MOTOR VISUAL — MODOS DE ECRÃ E AVISOS
 * ============================================================================
 */
void mudarModo(ModoEcra m, unsigned long agora) {
  if (vis.modo == m) return;
  Serial.printf("[UI] modo %s -> %s\n", nomeModo(vis.modo), nomeModo(m));
  vis.modo = m;
  vis.tModo = agora;
  if (m == ModoEcra::PROTETOR) iniciarProtetor(TipoProtetor::MARE, agora);
  if (m == ModoEcra::ACORDAR) olhos.tProximoPiscar = agora + DURACAO_ACORDAR_MS + 250;  // pisca ao acordar
}

void atualizarModo(unsigned long agora, float dtMs) {
  unsigned long t = agora - vis.tModo;
  bool interacaoRecente = emo.tUltimaInteracao != 0 && agora - emo.tUltimaInteracao < 1000;
  unsigned long inativo = agora - max(emo.tUltimaInteracao, vis.tModo);
  bool algoAcontecer = emo.transitoriaAtiva || emo.manualAtiva || rede.estado == EstadoRede::PORTAL || painel.ativo;
  if (painel.ativo && painel.arranque) {                          // ao ligar: checklist primeiro,
    vis.tModo = agora;                                            // depois as pálpebras abrem
    olhos.tProximoPiscar = agora + DURACAO_ACORDAR_MS + 250;
    return;
  }

  switch (vis.modo) {
    case ModoEcra::ACORDAR:
      if (t >= DURACAO_ACORDAR_MS) mudarModo(ModoEcra::ROSTO, agora);
      break;
    case ModoEcra::ROSTO:
      if (inativo > TEMPO_PROTETOR_MS && !algoAcontecer && !pessoaPresente(agora)) mudarModo(ModoEcra::ADORMECER, agora);
      break;
    case ModoEcra::ADORMECER:
      if (interacaoRecente || algoAcontecer) mudarModo(ModoEcra::ACORDAR, agora);
      else if (t >= DURACAO_ADORMECER_MS + 400) mudarModo(ModoEcra::PROTETOR, agora);
      break;
    case ModoEcra::PROTETOR:
      if (interacaoRecente || algoAcontecer) mudarModo(ModoEcra::ACORDAR, agora);
      break;
  }
  float alvo = (vis.modo == ModoEcra::PROTETOR) ? 1.0f : 0.0f;
  vis.alfaProtetor = suavizarExp(vis.alfaProtetor, alvo, dtMs,
                                 alvo > 0 ? DURACAO_FADE_PROT_MS / 3.0f : 120.0f);
}

// Ícone Wi-Fi: substitui o rosto ~4 s, SÓ quando o problema começa ou quando
// alguém chega (ver registarInteracao). Nada de repetição periódica.
float atualizarAviso(unsigned long agora, float* icone, const uint8_t*& cor) {
  bool problema = subs[(uint8_t)Sub::WIFI].saude == Saude::AVISO;
  if (!problema) { vis.avisoAtivo = false; vis.avisoPendente = false; return 0; }
  if (!vis.avisoAtivo && vis.avisoPendente && vis.modo == ModoEcra::ROSTO) {
    vis.avisoAtivo = true;
    vis.avisoPendente = false;
    vis.tInicioAviso = agora;
    vis.tUltimoAviso = agora;
  }
  if (!vis.avisoAtivo) return 0;
  unsigned long t = agora - vis.tInicioAviso;
  if (t >= DURACAO_AVISO_MS) { vis.avisoAtivo = false; return 0; }

  cor = (rede.estado == EstadoRede::PORTAL) ? COR_PORTAL : COR_PERDIDO;
  // Os arcos desenham-se um a um com fade (ponto -> médio -> grande).
  // Wi-Fi perdido: depois, os arcos maiores "perdem força" devagar. Sem piscar.
  for (int l = 0; l < 3; l++) {
    float v = suaveStep((t - 300.0f - l * 450.0f) / 500.0f);
    if (rede.estado == EstadoRede::PERDIDO && l > 0)
      v *= 1.0f - 0.75f * suaveStep((t - 2200.0f - (2 - l) * 300.0f) / 900.0f);
    for (int y = 0; y < ALTURA; y++) for (int x = 0; x < LARGURA; x++)
      if (ICONE_WIFI[l][y] & (0x80 >> x)) icone[y * LARGURA + x] = max(icone[y * LARGURA + x], v);
  }
  return min(suaveStep(t / 600.0f), suaveStep((DURACAO_AVISO_MS - t) / 600.0f));
}

/*
 * ============================================================================
 *  12. MOTOR VISUAL — COMPOSITOR (junta as camadas e escreve na matriz)
 * ============================================================================
 *  Porque não usar matriz.setBrightness() a cada frame (como na V6)?
 *   - A Adafruit documenta-o como "lossy" e para uso único no setup().
 *   - Com setBrightness(5) a lib escala por (c·6)>>8: a cor (40,40,40) vira
 *     (0,0,0) — o rosto ESPERA desaparecia entre batimentos.
 *  Aqui o brilho é aplicado por pixel em float, com piso de 1 nos canais
 *  ativos: o rosto NUNCA apaga. Ordem das camadas:
 *   desenho procedural -> crossfade -> escala (respiração) -> cor OKLCh
 *   -> aviso de rede -> protetor de ecrã -> quantização 8 bits.
 */
static inline uint8_t quantizar(float v) { return (uint8_t)limitar(lroundf(v), 0, 255); }

/*
 *  PAINEL DE ESTADO (camada 2) — 4 ícones, um por componente, cor = estado.
 *    ┌──────┬──────┐   Wi-Fi: barras de sinal   Movimento: cruz (acelerómetro)
 *    │ Wi-Fi│ Movim│   Solo: gota               Distância: emissor + onda
 *    ├──────┼──────┤   Verde = OK · Azul a pulsar = a iniciar · Âmbar a pulsar
 *    │ Solo │ Dist.│   = atenção · Vermelho fixo = erro · Cinzento = não instalado
 *    └──────┴──────┘   Com um culpado em destaque, os outros ficam a 30%.
 *  A página Web desenha exatamente os mesmos ícones (consistência).
 */
const uint8_t ICONES_SUB[(uint8_t)Sub::TOTAL][3] = {
  {0b001, 0b011, 0b111},     // Wi-Fi: barras de sinal
  {0b010, 0b111, 0b010},     // Acelerómetro: cruz de movimento
  {0b010, 0b111, 0b111},     // Solo: gota
  {0b001, 0b101, 0b001},     // Ultrassom: emissor + onda
};
const uint8_t ORIGEM_ICONE[(uint8_t)Sub::TOTAL][2] = {{0, 0}, {5, 0}, {0, 5}, {5, 5}};
const uint8_t COR_SAUDE[6][3] = {
  {0, 200, 70},     // OK: verde
  {0, 110, 255},    // A_LIGAR: azul
  {60, 60, 60},     // AUSENTE: cinzento
  {22, 22, 22},     // DESATIVADO: cinzento escuro
  {255, 130, 0},    // AVISO: âmbar
  {255, 0, 0},      // ERRO: vermelho
};

static float pulsoLento(unsigned long agora) {
  return 0.30f + 0.70f * (0.5f - 0.5f * cosf(2.0f * PI * (agora % PERIODO_PULSO_IND_MS) / PERIODO_PULSO_IND_MS));
}

// Devolve o alfa do painel (0 = invisível) e preenche rgb[64][3] (0..255)
float renderizarPainel(unsigned long agora, float rgb[][3]) {
  if (!painel.ativo) return 0;
  unsigned long t = agora - painel.tInicio;

  if (painel.arranque) {                                          // fim dinâmico do checklist
    bool aLigar = false;
    for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL; i++) aLigar |= subs[i].saude == Saude::A_LIGAR;
    if (aLigar) painel.tTudoResolvido = 0;
    else if (!painel.tTudoResolvido) painel.tTudoResolvido = agora;
    bool pronto = t >= ARRANQUE_MIN_MS && painel.tTudoResolvido && agora - painel.tTudoResolvido >= 800;
    if (pronto || t >= ARRANQUE_MAX_MS) { painel.duracao = t + 400; painel.arranque = false; }   // 400 ms de saída
  }
  if (!painel.arranque && t >= painel.duracao) { painel.ativo = false; return 0; }

  for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL; i++) {
    Saude sd = subs[i].saude;
    float a = (sd == Saude::A_LIGAR || sd == Saude::AVISO) ? pulsoLento(agora) : 1.0f;
    if (painel.destaque >= 0 && painel.destaque != (int8_t)i) a *= 0.30f;
    a *= suaveStep((float)((long)t - (long)(i * ARRANQUE_ESCALONADO_MS)) / 200.0f);   // acendem um a um
    for (uint8_t r = 0; r < 3; r++) for (uint8_t c = 0; c < 3; c++) {
      if (!(ICONES_SUB[i][r] & (0b100 >> c))) continue;
      int px = (ORIGEM_ICONE[i][1] + r) * LARGURA + ORIGEM_ICONE[i][0] + c;
      for (int k = 0; k < 3; k++) rgb[px][k] = COR_SAUDE[(uint8_t)sd][k] * a;
    }
  }
  float entrada = suaveStep(t / 250.0f);
  float saida = painel.arranque ? 1.0f : suaveStep(((long)painel.duracao - (long)t) / 400.0f);
  return min(entrada, saida);
}

// Anel de progresso na moldura enquanto o BOOT está premido (feedback do "segurar")
float progressoAnel(unsigned long agora) {
  if (!botao.premido || botao.longoFeito) return -1;
  unsigned long t = agora - botao.tPressao;
  if (botao.modoGirar) {                                          // segurar para fixar: anel rápido
    if (t < 300) return -1;
    return limitar((t - 300) / (float)(SEGURAR_FIXAR_MS - 300), 0, 1);
  }
  if (t < BOTAO_CURTO_MAX_MS) return -1;                          // toque curto: sem anel
  return limitar((t - BOTAO_CURTO_MAX_MS) / (float)(BOTAO_LONGO_MS - BOTAO_CURTO_MAX_MS), 0, 1);
}
void posicaoMoldura(int k, uint8_t& x, uint8_t& y) {             // 28 pixels, sentido horário
  if (k < 8)       { x = k;       y = 0; }
  else if (k < 15) { x = 7;       y = k - 7; }
  else if (k < 22) { x = 21 - k;  y = 7; }
  else             { x = 0;       y = 28 - k; }
}

void abrirPortalWifi(unsigned long agora) {
  // O portal usa a porta 80: libertar primeiro o nosso servidor.
  if (rede.servidorAtivo) { server.stop(); MDNS.end(); rede.servidorAtivo = false; }
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.startConfigPortal(NOME_AP_CONFIG);                           // não-bloqueante (process())
  registarInteracao(agora);
  Serial.printf("[BOTAO] Portal de configuracao aberto: \"%s\" (%d s)\n", NOME_AP_CONFIG, PORTAL_TIMEOUT_S);
}

/*
 *  BOTÃO BOOT — gramática de gestos (poucos, consistentes, com feedback):
 *   Normal:      toque = painel de estado (abre/fecha)
 *                duplo toque = entrar no MODO GIRAR (ícone ↻)
 *                segurar 5 s = reabrir configuração Wi-Fi (anel na moldura)
 *   Modo girar:  toque = girar o rosto 90° ↻ (imediato)
 *                segurar 1,5 s = fixar/soltar (cadeado) e sair
 *                10 s sem tocar = sai sozinho (as mudanças já ficaram guardadas)
 *  Custo assumido: o toque simples espera 350 ms para distinguir do duplo.
 */
void toqueSimples(unsigned long agora) {
  if (painel.ativo && !painel.arranque) painel.ativo = false;     // segundo toque fecha
  else abrirPainel(agora, DURACAO_PAINEL_MS, -1);
}
void atualizarBotao(unsigned long agora) {
  bool leitura = digitalRead(PINO_BOTAO) == LOW;
  if (leitura != botao.leituraAnterior) { botao.leituraAnterior = leitura; botao.tMudanca = agora; }

  // toque simples confirmado (não veio um segundo a tempo)
  if (botao.toquePendente && !botao.premido && agora - botao.tToque > DUPLO_TOQUE_MS) {
    botao.toquePendente = false;
    toqueSimples(agora);
  }
  if (botao.modoGirar && !botao.premido && agora - botao.tUltimaAcaoGirar > MODO_GIRAR_INATIVO_MS) {
    botao.modoGirar = false;
    Serial.println("[BOTAO] modo girar: saida por inatividade");
  }

  if (agora - botao.tMudanca < BOTAO_DEBOUNCE_MS || leitura == botao.premido) {
    if (botao.premido && !botao.longoFeito) {
      unsigned long t = agora - botao.tPressao;
      if (botao.modoGirar && t >= SEGURAR_FIXAR_MS) {            // modo girar: segurar = fixar/soltar
        botao.longoFeito = true;
        botao.modoGirar = false;
        fixarRotacao(rot.automatica, agora);                      // alterna
      } else if (!botao.modoGirar && t >= BOTAO_LONGO_MS) {
        botao.longoFeito = true;
        abrirPortalWifi(agora);
      }
    }
    return;
  }
  botao.premido = leitura;
  if (leitura) {                                                  // carregou
    botao.tPressao = agora;
    botao.longoFeito = false;
    registarInteracao(agora);                                     // acorda a planta
    return;
  }
  if (botao.longoFeito || agora - botao.tPressao >= BOTAO_CURTO_MAX_MS) return;   // largou após segurar
  // toque curto
  if (botao.modoGirar) {
    girarRosto(+1, agora);
    botao.tUltimaAcaoGirar = agora;
    return;
  }
  if (botao.toquePendente && agora - botao.tToque <= DUPLO_TOQUE_MS) {    // duplo toque
    botao.toquePendente = false;
    botao.modoGirar = true;
    botao.tUltimaAcaoGirar = agora;
    if (painel.ativo && !painel.arranque) painel.ativo = false;
    mostrarIconeBreve(ICONE_GIRAR, 160, 190, 255, agora);
    Serial.println("[BOTAO] modo girar: toque = girar 90 graus, segurar 1,5 s = fixar/soltar");
    return;
  }
  botao.toquePendente = true;
  botao.tToque = agora;
}

// Indicador de sistema (canal próprio): cor + padrão, nunca pisca
const uint8_t COR_IND_INFO[3]  = {0, 110, 255};    // azul: a iniciar
const uint8_t COR_IND_AVISO[3] = {255, 130, 0};    // âmbar: atenção
const uint8_t COR_IND_ERRO[3]  = {255, 0, 0};      // vermelho: erro
void desenharIndicador(unsigned long agora, float geral, float alfaProt) {
  Nivel n = nivelGeral();
  if (n == Nivel::OK) return;                                     // sem notícias = boas notícias
  if (n == Nivel::INFO && alfaProt > 0.5f) return;                // a dormir: só o que importa
  float a = 1.0f;                                                 // ERRO: fixo
  if (n != Nivel::ERRO)                                           // INFO/AVISO: pulso lento
    a = 0.30f + 0.70f * (0.5f - 0.5f * cosf(2.0f * PI * (agora % PERIODO_PULSO_IND_MS) / PERIODO_PULSO_IND_MS));
  const uint8_t* c = n == Nivel::ERRO ? COR_IND_ERRO : (n == Nivel::AVISO ? COR_IND_AVISO : COR_IND_INFO);
  float b = BRILHO_INDICADOR * geral * a;
  int i = INDICADOR_Y * LARGURA + INDICADOR_X;
  uint8_t q[3];
  for (int k = 0; k < 3; k++) { float v = c[k] * b; q[k] = (c[k] && v < 1.0f) ? 1 : quantizar(v); }
  for (int k = 0; k < 3; k++) vista[i][k] = q[k];
}

// DITHERING TEMPORAL (modulação sigma-delta), como o FastLED.
// Com brilho baixo, um canal pode valer só 2-6 níveis: passar de 3 para 4 é
// +33% e a respiração "treme". Aqui cada LED guarda o erro de arredondamento
// e soma-o no frame seguinte: alternando 3,4,3,4... a 100 FPS, o olho integra
// e vê 3,5. A média temporal é exata e o tremor desaparece.
float erroDither[NUM_LEDS][3];
static inline uint8_t quantizarComDither(float v, int i, int k) {
#if USAR_DITHERING
  float alvo = limitar(v, 0, 255) + erroDither[i][k];
  uint8_t q = (uint8_t)limitar(floorf(alvo + 0.5f), 0, 255);
  if (v < 0.5f) { q = 0; erroDither[i][k] = 0; return q; }       // apagado é apagado (sem "fantasmas")
  erroDither[i][k] = limitar(alvo - q, -0.5f, 0.5f);
  return q;
#else
  (void)i; (void)k;
  return quantizar(v);
#endif
}

bool quadroPronto = false;
// Saída final: copia a vista para o espelho da página e envia-a para os LEDs
// RODADA conforme a orientação. Em repouso (0/90/180/270°) é uma permutação
// exata dos pixels; durante a animação usa amostragem bilinear (giro suave).
void enviarParaLeds(unsigned long agora) {
  if (rot.animando) {
    float t = suaveStep((agora - rot.tInicioAnim) / (float)DURACAO_ANIM_ROTACAO_MS);
    rot.anguloExibido = misturar(rot.anguloInicio, rot.anguloAlvo, t);
    if (t >= 1.0f) { rot.animando = false; rot.anguloExibido = rot.quarto * 90.0f; }
  }
  for (int i = 0; i < NUM_LEDS; i++) for (int k = 0; k < 3; k++) espelho[i][k] = quantizar(vista[i][k]);

  const float f = rot.anguloExibido * PI / 180.0f, cf = cosf(f), sf = sinf(f);
  const float c = LARGURA / 2.0f;
  const bool exato = !rot.animando;
  for (uint8_t y = 0; y < ALTURA; y++) {
    for (uint8_t x = 0; x < LARGURA; x++) {
      // pixel físico -> ponto da vista: q = R(-φ)·(p - c) + c
      float px = x + 0.5f - c, py = y + 0.5f - c;
      float qx = cf * px + sf * py + c, qy = -sf * px + cf * py + c;
      float v[3] = {0, 0, 0};
      if (exato) {
        int sx = (int)floorf(qx), sy = (int)floorf(qy);
        if (sx >= 0 && sx < LARGURA && sy >= 0 && sy < ALTURA)
          for (int k = 0; k < 3; k++) v[k] = vista[sy * LARGURA + sx][k];
      } else {
        float fx = qx - 0.5f, fy = qy - 0.5f;
        int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
        float ax = fx - x0, ay = fy - y0;
        const float w[4] = {(1 - ax) * (1 - ay), ax * (1 - ay), (1 - ax) * ay, ax * ay};
        const int dx[4] = {0, 1, 0, 1}, dy[4] = {0, 0, 1, 1};
        for (int n = 0; n < 4; n++) {
          int sx = x0 + dx[n], sy = y0 + dy[n];
          if (sx < 0 || sx >= LARGURA || sy < 0 || sy >= ALTURA) continue;
          for (int k = 0; k < 3; k++) v[k] += w[n] * vista[sy * LARGURA + sx][k];
        }
      }
      int i = y * LARGURA + x;                                    // índice físico (dithering)
      matriz.setPixelColor(indicePixel(x, y), quantizarComDither(v[0], i, 0),
                           quantizarComDither(v[1], i, 1), quantizarComDither(v[2], i, 2));
    }
  }
  quadroPronto = true;                                            // show() é chamado no loop()
}

void renderizarFrame(unsigned long agora) {
  float dtMs = vis.tUltimoFrame ? (float)(agora - vis.tUltimoFrame) : (float)INTERVALO_FRAME_MS;
  dtMs = min(dtMs, 100.0f);
  vis.tUltimoFrame = agora;

  atualizarModo(agora, dtMs);
  atualizarOlhos(dtMs, agora);

  const StatusHardware st = statusGeral();
  const Expressao& ex = expressaoDe(emo.atual);
  Respiracao resp = atualizarRespiracao(ex, st, dtMs);
  vis.escala = resp.escala;
  const float geral = brilhoGeralPct / 100.0f;                   // botão de brilho global
  const float fator = resp.fatorBrilho * geral;

  // Cor = SÓ a emoção (paleta). O estado do hardware tem canal próprio
  // (indicador no canto) — ver secção 4b.
  float alvo[3];
  corEmocao(emo.atual, alvo);
  float cor[3];
  suavizarCor(alvo, dtMs, cor);

  // Rosto atual + anterior (crossfade suave entre expressões)
  bool olhosX = imu.status == StatusHardware::ERRO;
  // Buffers ESTÁTICOS (fora da pilha): a pilha do loop é preciosa (ver topo)
  static float desenho[NUM_LEDS], anterior[NUM_LEDS], rosto[NUM_LEDS], icone[NUM_LEDS];
  static float rgbPainel[NUM_LEDS][3];
  memset(desenho, 0, sizeof(desenho));
  desenharRosto(desenho, ex, olhosX, agora);
  float tCross = suaveStep((agora - vis.tInicioCrossfade) / (float)DURACAO_CROSSFADE_MS);
  if (tCross < 1.0f) {
    memset(anterior, 0, sizeof(anterior));
    desenharRosto(anterior, expressaoDe(vis.emocaoAnterior), olhosX, agora);
    for (int i = 0; i < NUM_LEDS; i++) desenho[i] = misturar(anterior[i], desenho[i], tCross);
  }
  escalarRosto(desenho, rosto, vis.escala);                       // respiração = tamanho

  // Aviso de rede
  memset(icone, 0, sizeof(icone));
  const uint8_t* corAviso = COR_PORTAL;
  float alfaAviso = atualizarAviso(agora, icone, corAviso);

  // Painel de estado (camada 2)
  memset(rgbPainel, 0, sizeof(rgbPainel));
  float alfaPainel = renderizarPainel(agora, rgbPainel);

  // Protetor
  float alfaCiclo = 0;
  if (vis.alfaProtetor > 0.01f) alfaCiclo = renderizarProtetor(agora, dtMs);

  // Teste de cores: toda a matriz numa cor primária pura (ignora rosto/paleta)
  int8_t fase = faseTesteCor(agora);
  if (fase >= 0) {
    uint8_t nivel = (uint8_t)max(12.0f, 120.0f * brilhoGeralPct / 100.0f);
    for (int i = 0; i < NUM_LEDS; i++) for (int k = 0; k < 3; k++) vista[i][k] = (k == fase) ? nivel : 0;
    enviarParaLeds(agora);
    return;
  }
  float alfaProt = vis.alfaProtetor;

  for (uint8_t y = 0; y < ALTURA; y++) {
    for (uint8_t x = 0; x < LARGURA; x++) {
      int i = y * LARGURA + x;
      float c[3];
      for (int k = 0; k < 3; k++) {
        float v = cor[k] * rosto[i] * fator;
        if (rosto[i] > 0.5f && cor[k] >= 4.0f && v < 1.0f && alfaProt < 0.5f) v = 1.0f;  // piso
        if (alfaAviso > 0) v = misturar(v, corAviso[k] * icone[i] * BRILHO_AVISO * geral, alfaAviso);
        if (alfaProt > 0.01f) v = misturar(v, prot.rgb[i][k] * BRILHO_PROTETOR * geral * alfaCiclo, alfaProt);
        if (alfaPainel > 0) {
          float pv = rgbPainel[i][k] * BRILHO_PAINEL * geral;
          if (rgbPainel[i][k] >= 4.0f && pv < 1.0f) pv = 1.0f;     // ícone nunca some
          v = misturar(v, pv, alfaPainel);
        }
        c[k] = v;
      }
      for (int k = 0; k < 3; k++) vista[i][k] = c[k];
    }
  }
  if (alfaPainel < 0.5f) desenharIndicador(agora, geral, alfaProt);   // o painel já diz tudo

  // Modo girar ativo: 4 cantos acesos a pulsar = "estou à espera do botão"
  if (botao.modoGirar) {
    float a = pulsoLento(agora);
    const uint8_t cantos[4][2] = {{0, 0}, {7, 0}, {0, 7}, {7, 7}};
    for (auto& c : cantos) { int i = c[1] * LARGURA + c[0];
      vista[i][0] = max(1.0f, 160 * 0.25f * geral * a); vista[i][1] = max(1.0f, 190 * 0.25f * geral * a); vista[i][2] = max(1.0f, 255 * 0.25f * geral * a); }
  }
  // Ícone de confirmação (↻ ao entrar no modo; cadeado ao fixar/soltar)
  if (iconeBreve.bmp && agora - iconeBreve.tInicio < DURACAO_ICONE_MS) {
    unsigned long t = agora - iconeBreve.tInicio;
    float a = min(suaveStep(t / 150.0f), suaveStep((DURACAO_ICONE_MS - t) / 250.0f));
    for (int y = 0; y < ALTURA; y++) for (int x = 0; x < LARGURA; x++) {
      int i = y * LARGURA + x;
      bool on = iconeBreve.bmp[y] & (0x80 >> x);
      for (int k = 0; k < 3; k++) {
        float v = on ? max(1.0f, iconeBreve.cor[k] * 0.25f * geral) : 0.0f;
        vista[i][k] = misturar(vista[i][k], v, a);
      }
    }
  }

  // Anel de progresso do botão (por cima de tudo)
  float prog = progressoAnel(agora);
  if (prog >= 0) {
    float acesos = prog * 28.0f;
    for (int k = 0; k < 28; k++) {
      float a = limitar(acesos - k, 0, 1);
      if (a <= 0) break;
      uint8_t x, y; posicaoMoldura(k, x, y);
      int i = y * LARGURA + x;
      uint8_t q[3] = {quantizar(160 * a * 0.30f * geral + 1), quantizar(190 * a * 0.30f * geral + 1), quantizar(255 * a * 0.30f * geral + 1)};
      for (int kk = 0; kk < 3; kk++) vista[i][kk] = q[kk];
    }
  }
  enviarParaLeds(agora);
}

/*
 * ============================================================================
 *  13. MÓDULO REDE & SERVIDOR WEB
 * ============================================================================
 *  O portal do WiFiManager usa a porta 80. Abrir o nosso WebServer na mesma
 *  porta ao mesmo tempo falha. Por isso o servidor só arranca quando há
 *  ligação e o portal está fechado.
 */
const char PAGINA_HTML[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="pt"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Mangue Interativo</title>
<style>:root{--fundo:#0d1a14;--cartao:#14271e;--botao:#1f3d2f;--borda:#2f5c46;--acento:#7fe0a8}
body{font-family:system-ui,sans-serif;background:var(--fundo);color:#e6f2ec;max-width:520px;margin:auto;padding:16px;transition:background 1.2s}
h1{font-size:1.4em;margin:.2em 0;color:var(--acento);transition:color 1.2s}.card{background:var(--cartao);border-radius:12px;padding:12px;margin:10px 0;transition:background 1.2s}
#espelho{display:grid;grid-template-columns:repeat(8,1fr);gap:4px;background:#050505;padding:8px;border-radius:10px;max-width:240px;margin:auto}
#espelho i{aspect-ratio:1;border-radius:50%;background:#151515}
#painelWeb{display:grid;grid-template-columns:repeat(8,11px);gap:2px;background:#050505;padding:6px;border-radius:8px}
#painelWeb i{width:11px;height:11px;border-radius:50%;background:#151515}
.selo{font-size:.55em;vertical-align:middle;padding:3px 10px;border-radius:12px;background:#333;color:#fff;margin-left:6px}
.offline{display:none;background:#5a3a00;color:#ffe2b0;border-radius:8px;padding:10px;margin:8px 0}
.seg button.on{background:var(--acento);color:#0b1410;font-weight:bold}
.lin{display:flex;gap:10px;align-items:flex-start;padding:7px 0;border-bottom:1px solid rgba(255,255,255,.08)}
.lin .ic{font-size:1.2em;width:1.4em;text-align:center}.lin small{opacity:.8}
details{margin-top:8px}summary{cursor:pointer}
.g{display:grid;grid-template-columns:1fr 1fr;gap:6px}b{color:var(--acento)}
button{background:var(--botao);color:#e6f2ec;border:1px solid var(--borda);border-radius:8px;padding:10px;font-size:1em;cursor:pointer;transition:background 1.2s}
button:hover{background:#2a5240}.bar{height:10px;background:#0a120e;border-radius:5px;overflow:hidden}
.bar i{display:block;height:100%;background:linear-gradient(90deg,#c0392b,#f1c40f,#2ecc71)}</style></head><body>
<h1>🌱 Mangue Interativo <span id="selo" class="selo">…</span></h1>
<div class="offline" id="offline">⚠️ Sem ligação à planta. Ela pode ter perdido o Wi-Fi — veja o canto da matriz (âmbar = problema de rede).</div>
<div class="card"><div id="espelho"></div>
<p style="text-align:center;margin:6px 0 0"><small>Espelho ao vivo da matriz</small></p>
</div>
<div class="card"><p>Saúde do sistema</p>
<div style="display:flex;gap:14px;align-items:center;margin-bottom:6px"><div id="painelWeb"></div>
<div><small>É isto que a placa mostra no <b>painel de estado</b>:<br>Wi-Fi ↖ · Movimento ↗ · Solo ↙ · Distância ↘<br>
<b>Botão BOOT:</b> toque = painel · manter 5 s = configurar Wi-Fi</small><br>
<button style="margin-top:6px" onclick="post('/api/painel')">Mostrar na placa</button></div></div>
<div id="saude"></div>
<small>Canto inferior direito da matriz: <b>apagado</b> = tudo bem · <span style="color:#5aa0ff">azul a pulsar</span> = a iniciar ·
<span style="color:#ffb040">âmbar a pulsar</span> = atenção · <span style="color:#ff5050">vermelho fixo</span> = erro.</small>
<details><summary><small>Histórico</small></summary><div id="eventos"><small>sem eventos</small></div></details></div>
<div class="card"><div class="g">
<div>Emoção: <b id="emocao">-</b></div><div>Modo: <b id="modo">-</b></div>
<div>Sistema: <b id="status">-</b></div><div>Wi-Fi: <b id="rede">-</b></div>
<div>Solo: <b id="solo">-</b></div><div>Distância: <b id="dist">-</b></div>
<div>Inclinação: <b id="incl">-</b></div><div>Respiração: <b id="resp">-</b></div></div>
<p>Energia</p><div class="bar"><i id="energia" style="width:0"></i></div></div>
<div class="card"><p>Mudar emoção (10 s)</p><div class="g" id="botoes"></div>
<p></p><div class="g"><button onclick="acao('auto')">Automático</button><button onclick="post('/api/acordar')">Acordar</button>
<button onclick="post('/api/protetor')">Protetor</button><button onclick="post('/api/calibrar')">Calibrar repouso</button></div>
<p>Brilho geral: <b id="brv">-</b></p><input type="range" id="brilho" min="5" max="100" step="5" style="width:100%"
oninput="$('brv').textContent=this.value+'%'" onchange="post('/api/brilho?valor='+this.value)"></div>
<div class="card"><p>Paleta de cores</p>
<select id="paleta" onchange="mudarPaleta()" style="width:100%;padding:8px;font-size:1em;background:var(--botao);color:#e6f2ec;border:1px solid var(--borda);border-radius:8px">
<option value="mangue">Mangue (original)</option><option value="oceano">Oceano</option><option value="por_do_sol">Pôr do sol</option>
<option value="pastel">Pastel</option><option value="floresta">Floresta</option><option value="cor_unica">Cor única (escolher)</option></select>
<p id="linhaCor" style="display:none">Cor: <input type="color" id="cor" onchange="mudarPaleta()"></p>
<div id="amostras" style="display:grid;grid-template-columns:repeat(9,1fr);gap:4px;margin-top:8px"></div>
<small>As cores do rosto mostram só emoções. Avisos do sistema aparecem no canto da matriz e em "Saúde do sistema".</small></div>
<div class="card"><p>Orientação do rosto</p>
<div class="g seg"><button id="btnAuto" onclick="post('/api/rotacao?auto=1')">🔄 Automática</button><button id="btnFixa" onclick="post('/api/rotacao?auto=0')">🔒 Fixa</button></div>
<p><small id="rotDica">-</small></p>
<div class="g"><button style="font-size:1.4em" title="Girar 90° anti-horário" onclick="post('/api/girar?sentido=-1')">↺</button>
<button style="font-size:1.4em" title="Girar 90° horário" onclick="post('/api/girar?sentido=1')">↻</button></div>
<p><small><b>Na placa:</b> duplo toque no BOOT = modo girar (cantos acesos) · toque = ↻ · segurar 1,5 s = 🔒 fixar/soltar.</small></p>
<p>Posição: <b id="rotPos">-</b> · <small id="rotMed">-</small></p>
<p><small id="rotEix">-</small></p>
<button onclick="calibrarOri()">Calibração completa (2 passos, deteta montagem espelhada)</button>
<div id="assist" style="display:none;margin-top:10px;padding:10px;border:1px solid var(--borda);border-radius:8px">
<div id="assistIco" style="font-size:2.2em;text-align:center">🙂</div>
<p id="assistTxt"></p>
<div class="g"><button id="assistOk" onclick="passoOri()">Confirmar</button><button id="assistFim" onclick="fecharOri()">Cancelar</button></div></div>
<p style="margin-top:8px"><small><a href="#" style="color:var(--acento)" onclick="post('/api/orientacao?passo=repor');return false">Repor montagem de fábrica</a></small></p></div>
<div class="card"><p>Cores dos LEDs</p>
<p><small>Se a placa mostra cores diferentes do espelho, a ordem das cores está errada.
Carregue em <b>Testar cores</b>: a placa deve mostrar VERMELHO, depois VERDE, depois AZUL.</small></p>
<div class="g"><button onclick="testarCores()">Testar cores</button>
<select id="ordem" onchange="post('/api/ordem?nome='+this.value)" style="padding:8px;font-size:1em;background:var(--botao);color:#e6f2ec;border:1px solid var(--borda);border-radius:8px">
<option>RGB</option><option>GRB</option><option>BRG</option><option>RBG</option><option>GBR</option><option>BGR</option></select></div>
<p id="testeTxt" style="font-size:1.3em;text-align:center;font-weight:bold;min-height:1.5em"></p>
<small id="dica" style="display:none">Viu outra cor? Use a tabela: se no "VERMELHO" a placa mostrou <b>verde</b> e no "VERDE" mostrou <b>vermelho</b>, troque RGB&harr;GRB.
Se mostrou <b>azul</b> no "VERMELHO", experimente BRG ou BGR. Escolha e teste de novo.</small></div>
<div class="card"><small id="sis">-</small></div>
<script>
const E=['espera','alegria','sede','susto','amor','raiva','sono','curioso','confuso'];
const B=document.getElementById('botoes');E.forEach(e=>{const b=document.createElement('button');b.textContent=e;b.onclick=()=>acao(e);B.appendChild(b)});
function post(u){return fetch(u,{method:'POST'}).then(atualizar)}
function acao(e){return post('/api/emocao?nome='+e)}
function $(i){return document.getElementById(i)}
// ---- assistente de calibração da orientação
let passoAtual=1;
const PASSOS={1:['🙂','<b>Passo 1 de 2.</b> Segure a placa <b>de pé</b>, com o rosto <b>direito</b> (boca para baixo, como deve ficar). Mantenha-a parada e confirme.'],
2:['🫤↺','<b>Passo 2 de 2.</b> Rode a placa <b>90° para a ESQUERDA</b> (o lado esquerdo do rosto fica para baixo). Mantenha parada e confirme.']};
function mostrarPasso(n,extra){passoAtual=n;$('assist').style.display='block';$('assistFim').textContent='Cancelar';$('assistIco').textContent=PASSOS[n][0];$('assistTxt').innerHTML=PASSOS[n][1]+(extra?'<br><small style="color:#ffb040">'+extra+'</small>':'');$('assistOk').style.display='block'}
function calibrarOri(){mostrarPasso(1)}
function fecharOri(){$('assist').style.display='none'}
async function passoOri(){try{const r=await(await fetch('/api/orientacao?passo='+passoAtual,{method:'POST'})).json();
if(r.estado=='passo2')mostrarPasso(2);
else if(r.estado=='concluido'){$('assistIco').textContent='✅';$('assistTxt').innerHTML=r.msg+'<br><small>Rode a placa: o rosto deve ficar sempre direito.</small>';$('assistOk').style.display='none';$('assistFim').textContent='Fechar'}
else if(r.estado=='passo1')mostrarPasso(1,r.msg);
else mostrarPasso(passoAtual,'Ainda não: '+r.msg+'.')}catch(e){mostrarPasso(passoAtual,'Sem ligação à planta.')}}
const NOMES_TESTE=[['VERMELHO','#ff3b3b'],['VERDE','#3bff6a'],['AZUL','#4a7dff']];
function testarCores(){post('/api/teste_cor');$('dica').style.display='block'}
// ---- tema da página a partir da paleta (cor de ESPERA = identidade da paleta)
function hex2rgb(h){const n=parseInt(h.slice(1),16);return[n>>16&255,n>>8&255,n&255]}
function mix(c,f,base){return 'rgb('+c.map((v,i)=>Math.round(base[i]+(v-base[i])*f)).join(',')+')'}
function aplicarTema(s){const id=hex2rgb(s.cores[0]),ac=hex2rgb(s.cores[Math.max(0,E.indexOf(s.emocao))]),r=document.documentElement.style,k=[8,8,10];
r.setProperty('--fundo',mix(id,.10,k));r.setProperty('--cartao',mix(id,.20,k));r.setProperty('--botao',mix(id,.32,k));
r.setProperty('--borda',mix(id,.55,k));const m=Math.max(...ac,1);r.setProperty('--acento','rgb('+ac.map(v=>Math.round(80+175*v/m)).join(',')+')')}
// ---- Saúde do sistema: o QUÊ, desde QUANDO e o que FAZER (Nielsen #9)
const ICONE={ok:'✅',a_ligar:'⏳',ausente:'➖',desativado:'⏸️',aviso:'⚠️',erro:'⛔'};
const ROTULO={ok:'OK',a_ligar:'A iniciar',ausente:'Não instalado',desativado:'Desativado',aviso:'Atenção',erro:'Erro'};
const SELO={ok:['Tudo bem','#1f7a45'],info:['A iniciar','#1f5fa8'],aviso:['Atenção','#a86400'],erro:['Erro','#a82020']};
const NOME={imu:'Acelerómetro (QMI8658)',wifi:'Wi-Fi',solo:'Humidade do solo',ultrassom:'Distância (HC-SR04)'};
const TXT={
imu:{ok:'A funcionar.',a_ligar:'A iniciar o sensor…',erro:'Não responde no I2C (SDA 11 / SCL 12). Reinicie a placa; se continuar, o sensor pode estar danificado. A planta mostra olhos em X.'},
wifi:{ok:'Ligado.',a_ligar:'A ligar à rede guardada…',aviso_portal:'Precisa de configuração: ligue-se à rede "Mangue_Interativo_AP" e escolha a sua rede.',
aviso_perdido:'Ligação perdida. A planta tenta religar sozinha. Verifique o router ou a distância.',aviso:'Problema de rede.'},
solo:{ok:'A ler a humidade.',a_ligar:'À espera da primeira leitura…',ausente:'Sem leitura válida no GPIO4 — normal se o sensor não estiver ligado. A planta não sente sede.',desativado:'Desativado no código (USAR_SENSOR_SOLO 0).'},
ultrassom:{ok:'A medir a distância.',ausente:'Sem eco no GPIO6 — normal se o sensor não estiver ligado. A planta não reage a aproximações.',desativado:'Desativado no código (USAR_ULTRASSOM 0).'}};
// Mesmos ícones e cores do firmware (ICONES_SUB / COR_SAUDE)
const ICS={wifi:[1,3,7],imu:[2,7,2],solo:[2,7,7],ultrassom:[1,5,1]},ORI={wifi:[0,0],imu:[5,0],solo:[0,5],ultrassom:[5,5]};
const CS={ok:'#00c846',a_ligar:'#006eff',ausente:'#5a5a5a',desativado:'#2a2a2a',aviso:'#ff8200',erro:'#ff0000'};
const PW=$('painelWeb');for(let i=0;i<64;i++)PW.appendChild(document.createElement('i'));
function painelWeb(subs){[...PW.children].forEach(c=>{c.style.background='#151515';c.style.boxShadow='none'});
subs.forEach(x=>{const b=ICS[x.id],o=ORI[x.id];for(let r=0;r<3;r++)for(let c=0;c<3;c++)if(b[r]&(4>>c)){
const el=PW.children[(o[1]+r)*8+o[0]+c];el.style.background=CS[x.estado];el.style.boxShadow='0 0 5px '+CS[x.estado]}})}
function tempo(s){return s<60?s+' s':s<3600?Math.floor(s/60)+' min':Math.floor(s/3600)+' h'}
async function saude(){try{const h=await(await fetch('/api/saude')).json();
const sl=SELO[h.nivel];$('selo').textContent=sl[0];$('selo').style.background=sl[1];painelWeb(h.subs);
$('saude').innerHTML=h.subs.map(x=>{let t=TXT[x.id][x.estado+(x.id=='wifi'&&x.estado=='aviso'?'_'+h.rede:'')]||TXT[x.id][x.estado]||'';
return '<div class="lin"><span class="ic">'+ICONE[x.estado]+'</span><div><b>'+NOME[x.id]+'</b> — '+ROTULO[x.estado]+' <small>(há '+tempo(x.ha_s)+')</small><br><small>'+t+'</small></div></div>'}).join('');
$('eventos').innerHTML=h.eventos.length?h.eventos.map(e=>'<div><small>há '+tempo(e.ha_s)+' · '+NOME[e.sub]+': '+ROTULO[e.de]+' → <b>'+ROTULO[e.para]+'</b></small></div>').join(''):'<small>sem eventos</small>';
}catch(e){}}
setInterval(saude,2000);
// ---- espelho da matriz (brilho normalizado para se ver bem no ecrã)
const ESP=$('espelho');for(let i=0;i<64;i++)ESP.appendChild(document.createElement('i'));
async function espelhar(){try{const t=await(await fetch('/api/matriz')).text();let m=1;const px=[];
for(let i=0;i<64;i++){const c=[0,1,2].map(k=>parseInt(t.substr(i*6+k*2,2),16));px.push(c);m=Math.max(m,...c)}
px.forEach((c,i)=>{const v=c.map(x=>Math.round(Math.pow(x/m,0.6)*255));ESP.children[i].style.background=(c[0]|c[1]|c[2])?'rgb('+v.join(',')+')':'#151515';
ESP.children[i].style.boxShadow=(c[0]|c[1]|c[2])?'0 0 8px rgb('+v.join(',')+')':'none'})}catch(e){}}
setInterval(espelhar,250);
function mudarPaleta(){const p=$('paleta').value;let u='/api/paleta?nome='+p;
if(p=='cor_unica')u+='&cor='+$('cor').value.substring(1);$('linhaCor').style.display=p=='cor_unica'?'block':'none';
fetch(u,{method:'POST'}).then(r=>r.json()).then(j=>{if(!j.ok)alert(j.erro)}).then(atualizar)}
async function atualizar(){try{const s=await(await fetch('/api/estado')).json();
$('emocao').textContent=s.emocao+(s.manual?' (manual)':'');$('modo').textContent=s.modo;
$('status').textContent=(SELO[s.nivel]||['-'])[0];$('rede').textContent=s.rede;
$('solo').textContent=s.solo.ativo?(s.solo.pct+'%'+(s.solo.sede?' sede':'')):'desligado';
$('dist').textContent=s.us.detectado?(s.us.cm>=999?'nada':s.us.cm.toFixed(0)+' cm'):'sem sensor';
$('incl').textContent=s.imu.inclX.toFixed(2)+' / '+s.imu.inclY.toFixed(2)+' g';$('resp').textContent=s.resp.toFixed(0)+' /min';
$('energia').style.width=s.energia+'%';
if(document.activeElement!==$('brilho')){$('brilho').value=s.brilho;$('brv').textContent=s.brilho+'%'}
if(document.activeElement!==$('paleta')&&document.activeElement!==$('cor')){$('paleta').value=s.paleta;$('cor').value=s.cor.toLowerCase();
$('linhaCor').style.display=s.paleta=='cor_unica'?'block':'none'}
if(document.activeElement!==$('ordem'))$('ordem').value=s.ordem;
$('btnAuto').classList.toggle('on',s.rot.auto);$('btnFixa').classList.toggle('on',!s.rot.auto);
$('rotDica').textContent=s.rot.auto?'O rosto gira sozinho quando um lado está a menos de 15° do horizonte. Ficou torto? Segure a placa direita e use ↺ ↻ até ficar direito: a planta aprende.':'Rosto fixo nesta posição (como o bloqueio de rotação do telemóvel). Use ↺ ↻ para escolher a posição.';
$('rotPos').textContent=s.rot.graus+'°'+(s.rot.auto?'':' (fixa)');
$('rotEix').textContent='Montagem do sensor: '+s.rot.eixos+'°'+(s.rot.espelho?' (espelhado)':'');
$('rotMed').textContent=s.rot.vertical?('chão medido a '+s.rot.medido+'°'):'placa deitada: orientação mantida';
const tt=$('testeTxt');if(s.teste>=0){tt.textContent='A placa deve estar: '+NOMES_TESTE[s.teste][0];tt.style.color=NOMES_TESTE[s.teste][1]}else tt.textContent='';
aplicarTema(s);falhas=0;$('offline').style.display='none';
$('amostras').innerHTML=s.cores.map((c,i)=>'<div title="'+E[i]+'" style="height:22px;border-radius:5px;background:'+c+(E[i]==s.emocao?';outline:2px solid #fff':'')+'"></div>').join('');
$('sis').textContent='uptime '+s.sistema.uptime_s+' s · RSSI '+s.sistema.rssi+' dBm · heap '+s.sistema.heap+' B · pilha livre '+s.sistema.pilha_livre+' B · loop máx '+s.sistema.loop_max_us+' µs · '+s.sistema.fps+' FPS · último reinício: '+s.sistema.reinicio+(s.sistema.crash_em?' em '+s.sistema.crash_em:'');
}catch(e){$('sis').textContent='sem ligação…';if(++falhas>=4)$('offline').style.display='block'}}
let falhas=0;setInterval(atualizar,500);atualizar();saude();
</script></body></html>)HTML";

void rotaRaiz() { server.send_P(200, "text/html; charset=utf-8", PAGINA_HTML); }

void rotaEstado() {
  static char json[1100];                                         // estático: fora da pilha
  snprintf(json, sizeof(json),
    "{\"emocao\":\"%s\",\"manual\":%s,\"modo\":\"%s\",\"status\":\"%s\",\"rede\":\"%s\",\"energia\":%.0f,\"resp\":%.1f,\"brilho\":%u,"
    "\"imu\":{\"status\":\"%s\",\"calibrado\":%s,\"inclX\":%.2f,\"inclY\":%.2f,\"agitacao\":%.2f,\"ok\":%lu,\"falhas\":%lu},"
    "\"us\":{\"detectado\":%s,\"cm\":%.0f,\"perto\":%s},"
    "\"solo\":{\"ativo\":%s,\"status\":\"%s\",\"mv\":%.0f,\"pct\":%d,\"sede\":%s},"
    "\"sistema\":{\"uptime_s\":%lu,\"rssi\":%d,\"heap\":%lu,\"loop_max_us\":%lu,\"fps\":%lu,\"pilha_livre\":%lu,\"reinicio\":\"%s\",\"crash_em\":\"%s\",\"crashes\":%u}",
    expressaoDe(emo.atual).nome, emo.manualAtiva ? "true" : "false", nomeModo(vis.modo),
    nomeStatus(statusGeral()), nomeRede(rede.estado), emo.energia, vis.ritmo, (unsigned)brilhoGeralPct,
    nomeStatus(imu.status), imu.calibrado ? "true" : "false", mov.inclX, mov.inclY, imu.agitacao,
    (unsigned long)imu.leiturasOk, (unsigned long)imu.leiturasFalhadas,
    us.detectado ? "true" : "false", us.distCm, pessoaPresente(millis()) ? "true" : "false",
    (USAR_SENSOR_SOLO && solo.presente) ? "true" : "false", nomeStatus(solo.status), solo.mvFiltrado, solo.percentagem,
    solo.comSede ? "true" : "false",
    millis() / 1000UL, (int)WiFi.RSSI(), (unsigned long)ESP.getFreeHeap(),
    (unsigned long)diag.loopMaxUsPublicado, (unsigned long)diag.fps, (unsigned long)diag.pilhaLivreMin,
    nomeMotivo(reinicio.motivo), reinicio.foiCrash ? NOMES_ETAPA[(uint8_t)reinicio.etapaCrash] : "",
    (unsigned)reinicio.crashesSeguidos);
  // Paleta: nome, cor única e as 9 cores (para a pré-visualização na página)
  size_t n = strlen(json);
  float atual[3]; oklchParaRgb(vis.corLch, atual);
  n += snprintf(json + n, sizeof(json) - n,
                ",\"rot\":{\"auto\":%s,\"graus\":%d,\"medido\":%d,\"vertical\":%s,\"eixos\":%d,\"espelho\":%s,\"modo_girar\":%s},\"ordem\":\"%s\",\"teste\":%d,\"nivel\":\"%s\",\"cor_atual\":\"#%02X%02X%02X\",\"paleta\":\"%s\",\"cor\":\"#%06lX\",\"cores\":[",
                rot.automatica ? "true" : "false", rot.quarto * 90, (int)(lroundf(rot.anguloMedido) % 360), rot.vertical ? "true" : "false",
                eixos.quarto * 90, eixos.espelho ? "true" : "false", botao.modoGirar ? "true" : "false",
                ORDENS_COR[ordemCor].nome, (int)faseTesteCor(millis()),
                NOMES_NIVEL[(uint8_t)nivelGeral()], (int)atual[0], (int)atual[1], (int)atual[2],
                NOMES_PALETA[(uint8_t)paletaAtual], (unsigned long)corUnica);
  for (uint8_t e = 0; e < (uint8_t)Emocao::TOTAL && n < sizeof(json); e++) {
    float c[3]; corEmocao((Emocao)e, c);
    n += snprintf(json + n, sizeof(json) - n, "%s\"#%02X%02X%02X\"", e ? "," : "", (int)c[0], (int)c[1], (int)c[2]);
  }
  if (n < sizeof(json)) snprintf(json + n, sizeof(json) - n, "]}");
  server.send(200, "application/json", json);
}

void rotaEmocao() {
  String nome = server.arg("nome");
  unsigned long agora = millis();
  registarInteracao(agora);
  if (nome.equalsIgnoreCase("auto")) { emo.manualAtiva = false; server.send(200, "application/json", "{\"ok\":true}"); return; }
  for (uint8_t i = 0; i < (uint8_t)Emocao::TOTAL; i++) {
    if (nome.equalsIgnoreCase(EXPRESSOES[i].nome)) {
      long dur = server.hasArg("duracao") ? server.arg("duracao").toInt() : DURACAO_MANUAL_S;
      dur = (long)limitar((float)dur, 1, 600);
      emo.manualAtiva = true;
      emo.manual = (Emocao)i;
      emo.fimManual = agora + (unsigned long)dur * 1000UL;
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"ok\":false,\"erro\":\"emocao desconhecida\"}");
}

void rotaAcordar()  { registarInteracao(millis()); server.send(200, "application/json", "{\"ok\":true}"); }
void rotaProtetor() {
  emo.manualAtiva = false;               // senão a emoção manual acordava-o no frame seguinte
  emo.transitoriaAtiva = false;
  mudarModo(ModoEcra::PROTETOR, millis());
  server.send(200, "application/json", "{\"ok\":true}");
}
void rotaBrilho() {
  long v = server.arg("valor").toInt();
  if (v < BRILHO_GERAL_MIN || v > BRILHO_GERAL_MAX) {
    server.send(400, "application/json", "{\"ok\":false,\"erro\":\"valor 5-100\"}");
    return;
  }
  brilhoGeralPct = (uint8_t)v;
  preferencias.putUChar("brilho", brilhoGeralPct);   // só grava quando o utilizador larga o slider
  server.send(200, "application/json", "{\"ok\":true}");
}
// POST /api/paleta?nome=oceano            -> paleta predefinida
// POST /api/paleta?nome=cor_unica&cor=FF8800 -> uma só cor para todas as emoções
void rotaPaleta() {
  String nome = server.arg("nome");
  int8_t idx = -1;
  for (uint8_t i = 0; i < (uint8_t)Paleta::TOTAL; i++) if (nome.equalsIgnoreCase(NOMES_PALETA[i])) idx = i;
  if (idx < 0) { server.send(400, "application/json", "{\"ok\":false,\"erro\":\"paleta desconhecida\"}"); return; }

  if (server.hasArg("cor")) {
    String hex = server.arg("cor");
    const char* h = hex.c_str();
    if (h[0] == '#') h++;
    char* fim = nullptr;
    unsigned long v = strtoul(h, &fim, 16);
    if (strlen(h) != 6 || *fim != '\0') {
      server.send(400, "application/json", "{\"ok\":false,\"erro\":\"cor deve ser RRGGBB\"}"); return;
    }
    uint8_t maior = max((v >> 16) & 0xFF, max((v >> 8) & 0xFF, v & 0xFF));
    if (maior < COR_MIN_CANAL_MAX) {   // quase preto: o rosto desapareceria
      server.send(400, "application/json", "{\"ok\":false,\"erro\":\"cor demasiado escura\"}"); return;
    }
    corUnica = (uint32_t)v;
    preferencias.putUInt("cor", corUnica);
  }
  paletaAtual = (Paleta)idx;
  preferencias.putUChar("paleta", (uint8_t)paletaAtual);
  registarInteracao(millis());           // acorda para mostrar a nova cor
  server.send(200, "application/json", "{\"ok\":true}");
}

// GET /api/matriz -> 64 pixels "RRGGBB" seguidos (384 caracteres), ordem linha a linha.
// A página desenha um espelho ao vivo do rosto: o que se vê lá é o que está nos LEDs.
// POST /api/teste_cor -> 6 s: vermelho, verde, azul puros
// POST /api/painel -> mostra o painel de estado na placa
void rotaPainel() {
  registarInteracao(millis());
  abrirPainel(millis(), DURACAO_PAINEL_MS, -1);
  server.send(200, "application/json", "{\"ok\":true}");
}

void rotaTesteCor() {
  testeCorAtivo = true;
  tInicioTesteCor = millis();
  registarInteracao(millis());
  server.send(200, "application/json", "{\"ok\":true}");
}

// ---- CALIBRAÇÃO GUIADA DA ORIENTAÇÃO (2 passos) ----------------------------
// Passo 1: placa de pé, rosto DIREITO (boca para baixo)        -> mede φ1
// Passo 2: placa rodada 90° para a ESQUERDA (lado esquerdo em baixo) -> φ2
// Hipótese rotação: φ = φcru + θ        -> θ = -φ1;  confere φ2 + θ ≈ 90°
// Hipótese espelho: φ = -φcru + θ'      -> θ' = φ1;  confere -φ2 + θ' ≈ 90°
// Escolhe a hipótese que acerta o passo 2 (erro <= 30°) e arredonda a 90°.
void responderCalibracao(int codigo, const char* estado, const char* msg) {
  char json[200];
  snprintf(json, sizeof(json), "{\"ok\":%s,\"estado\":\"%s\",\"msg\":\"%s\"}", codigo == 200 ? "true" : "false", estado, msg);
  server.send(codigo, "application/json", json);
}
void rotaCalibrarOrientacao() {
  String passo = server.arg("passo");
  unsigned long agora = millis();
  registarInteracao(agora);
  if (passo.equalsIgnoreCase("repor")) {
    eixos.passo1Feito = false;
    aplicarMontagem(CORRECAO_EIXOS_PADRAO, ESPELHO_EIXOS_PADRAO, agora);
    responderCalibracao(200, "reposto", "Montagem de fabrica reposta.");
    return;
  }
  const char* problema = condicaoMedicao();
  if (problema) { responderCalibracao(409, "repetir", problema); return; }
  float phi = anguloCru();
  if (passo.equalsIgnoreCase("1")) {
    eixos.phiPasso1 = phi;
    eixos.passo1Feito = true;
    responderCalibracao(200, "passo2", "Agora rode a placa 90 graus para a ESQUERDA (lado esquerdo do rosto para baixo) e confirme.");
    return;
  }
  if (passo.equalsIgnoreCase("2")) {
    if (!eixos.passo1Feito) { responderCalibracao(409, "passo1", "Faca primeiro o passo 1."); return; }
    float thetaRot = -eixos.phiPasso1, thetaEsp = eixos.phiPasso1;
    float erroRot = fabsf(difAngulo(phi + thetaRot, 90.0f));
    float erroEsp = fabsf(difAngulo(-phi + thetaEsp, 90.0f));
    bool espelho = erroEsp < erroRot;
    float erro = espelho ? erroEsp : erroRot;
    float theta = espelho ? thetaEsp : thetaRot;
    if (erro > 30.0f) {
      eixos.passo1Feito = false;
      responderCalibracao(409, "passo1", "As duas posicoes nao estao a 90 graus uma da outra. Recomece pelo passo 1.");
      return;
    }
    float thetaN = fmodf(theta + 720.0f, 360.0f);
    uint8_t q = (uint8_t)(lroundf(thetaN / 90.0f) & 3);
    eixos.passo1Feito = false;
    aplicarMontagem(q, espelho, agora);
    char msg[120];
    snprintf(msg, sizeof(msg), "Calibrado: sensor a %d graus%s (desvio %.0f graus).", q * 90, espelho ? " e espelhado" : "", erro);
    responderCalibracao(200, "concluido", msg);
    return;
  }
  responderCalibracao(400, "erro", "passo deve ser 1, 2 ou repor");
}

// POST /api/rotacao?auto=1|0 -> liga/desliga a rotação automática (guardado)
void rotaRotacao() {
  String v = server.arg("auto");
  if (!(v.equalsIgnoreCase("1") || v.equalsIgnoreCase("0"))) {
    server.send(400, "application/json", "{\"ok\":false,\"erro\":\"auto deve ser 0 ou 1\"}");
    return;
  }
  fixarRotacao(!v.equalsIgnoreCase("1"), millis());               // fixa = fica na posição ATUAL
  server.send(200, "application/json", "{\"ok\":true}");
}

// POST /api/girar?sentido=1|-1 -> ↻ / ↺ 90° (aprende a correção se automática)
void rotaGirar() {
  String v = server.arg("sentido");
  if (!(v.equalsIgnoreCase("1") || v.equalsIgnoreCase("-1"))) {
    server.send(400, "application/json", "{\"ok\":false,\"erro\":\"sentido deve ser 1 ou -1\"}");
    return;
  }
  girarRosto(v.toInt(), millis());
  server.send(200, "application/json", "{\"ok\":true}");
}

// POST /api/ordem?nome=GRB -> muda a ordem das cores dos LEDs e guarda
void rotaOrdem() {
  String nome = server.arg("nome");
  for (uint8_t i = 0; i < NUM_ORDENS_COR; i++) {
    if (nome.equalsIgnoreCase(ORDENS_COR[i].nome)) {
      ordemCor = i;
      matriz.updateType(ORDENS_COR[i].tipo + NEO_KHZ800);
      preferencias.putUChar("ordem", i);
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"ok\":false,\"erro\":\"ordem desconhecida\"}");
}

// GET /api/saude -> estado de cada componente + histórico (mais recente primeiro)
void rotaSaude() {
  static char json[1400];                                         // estático: fora da pilha
  unsigned long agora = millis();
  size_t n = snprintf(json, sizeof(json), "{\"nivel\":\"%s\",\"rede\":\"%s\",\"subs\":[",
                      NOMES_NIVEL[(uint8_t)nivelGeral()], nomeRede(rede.estado));
  for (uint8_t i = 0; i < (uint8_t)Sub::TOTAL && n < sizeof(json); i++)
    n += snprintf(json + n, sizeof(json) - n, "%s{\"id\":\"%s\",\"estado\":\"%s\",\"ha_s\":%lu}",
                  i ? "," : "", NOMES_SUB[i], NOMES_SAUDE[(uint8_t)subs[i].saude], (agora - subs[i].desde) / 1000UL);
  if (n < sizeof(json)) n += snprintf(json + n, sizeof(json) - n, "],\"eventos\":[");
  for (uint8_t k = 0; k < nEventos && n < sizeof(json); k++) {
    const Evento& e = eventos[(iEvento + MAX_EVENTOS - 1 - k) % MAX_EVENTOS];
    n += snprintf(json + n, sizeof(json) - n, "%s{\"ha_s\":%lu,\"sub\":\"%s\",\"de\":\"%s\",\"para\":\"%s\"}",
                  k ? "," : "", (agora - e.t) / 1000UL, NOMES_SUB[e.sub], NOMES_SAUDE[(uint8_t)e.de], NOMES_SAUDE[(uint8_t)e.para]);
  }
  if (n < sizeof(json)) snprintf(json + n, sizeof(json) - n, "]}");
  server.send(200, "application/json", json);
}

void rotaMatriz() {
  static const char HEX_[] = "0123456789ABCDEF";
  static char txt[NUM_LEDS * 6 + 1];
  for (int i = 0; i < NUM_LEDS; i++)
    for (int k = 0; k < 3; k++) {
      txt[i * 6 + k * 2]     = HEX_[espelho[i][k] >> 4];
      txt[i * 6 + k * 2 + 1] = HEX_[espelho[i][k] & 0xF];
    }
  txt[NUM_LEDS * 6] = '\0';
  server.send(200, "text/plain", txt);
}

void rotaCalibrar() { imu.calibrado = false; imu.amostrasCal = 0; server.send(200, "application/json", "{\"ok\":true}"); }

void configurarRotas() {
  server.on("/", HTTP_GET, rotaRaiz);
  server.on("/api/estado", HTTP_GET, rotaEstado);
  server.on("/api/emocao", HTTP_POST, rotaEmocao);
  server.on("/api/acordar", HTTP_POST, rotaAcordar);
  server.on("/api/protetor", HTTP_POST, rotaProtetor);
  server.on("/api/calibrar", HTTP_POST, rotaCalibrar);
  server.on("/api/brilho", HTTP_POST, rotaBrilho);
  server.on("/api/paleta", HTTP_POST, rotaPaleta);
  server.on("/api/matriz", HTTP_GET, rotaMatriz);
  server.on("/api/saude", HTTP_GET, rotaSaude);
  server.on("/api/teste_cor", HTTP_POST, rotaTesteCor);
  server.on("/api/painel", HTTP_POST, rotaPainel);
  server.on("/api/ordem", HTTP_POST, rotaOrdem);
  server.on("/api/rotacao", HTTP_POST, rotaRotacao);
  server.on("/api/girar", HTTP_POST, rotaGirar);
  server.on("/api/orientacao", HTTP_POST, rotaCalibrarOrientacao);
  server.onNotFound([]() { server.send(404, "text/plain", "404"); });
}

void gerirRede(unsigned long agora) {
  wm.process();                                                   // portal não-bloqueante
  if (rede.servidorAtivo) server.handleClient();
  if (!temporizador(tRede, INTERVALO_REDE_MS, agora)) return;

  bool ligado = WiFi.status() == WL_CONNECTED;
  EstadoRede antes = rede.estado;
  if (wm.getConfigPortalActive())   rede.estado = EstadoRede::PORTAL;
  else if (ligado)                  rede.estado = EstadoRede::LIGADO;
  else if (rede.jaLigou)            rede.estado = EstadoRede::PERDIDO;
  else                              rede.estado = EstadoRede::A_LIGAR;

  if (rede.estado == antes) return;
  Serial.printf("[REDE] %s -> %s\n", nomeRede(antes), nomeRede(rede.estado));

  if (rede.estado == EstadoRede::PERDIDO) rede.tPerda = agora;
  if (rede.estado == EstadoRede::LIGADO) {
    rede.jaLigou = true;
    if (!rede.servidorAtivo) {
      server.begin();
      if (MDNS.begin(NOME_MDNS)) MDNS.addService("http", "tcp", PORTA_SERVIDOR_WEB);
      rede.servidorAtivo = true;
      Serial.printf("[WEB] http://%s  ou  http://%s.local\n", WiFi.localIP().toString().c_str(), NOME_MDNS);
    }
  }
}

/*
 * ============================================================================
 *  14. DIAGNÓSTICO
 * ============================================================================
 */
void registarDiagnostico(uint32_t duracaoUs, unsigned long agora) {
  diag.loopMaxUs = max(diag.loopMaxUs, duracaoUs);
  if (agora - diag.tJanela >= 1000) {
    diag.pilhaLivreMin = uxTaskGetStackHighWaterMark(NULL);       // ESP-IDF: em bytes
    diag.fps = diag.frames; diag.frames = 0;
    diag.loopMaxUsPublicado = diag.loopMaxUs; diag.loopMaxUs = 0;
    diag.tJanela = agora;
  }
  if (temporizador(tLog, INTERVALO_LOG_MS, agora)) {
    Serial.printf("[ESTADO] %s | modo %s | sens %s | rede %s | energia %.0f | resp %.0f/min | incl %.2f/%.2f | dist %.0f | solo %d%% | loop max %lu us | %lu FPS\n",
                  expressaoDe(emo.atual).nome, nomeModo(vis.modo), nomeStatus(statusGeral()), nomeRede(rede.estado),
                  emo.energia, vis.ritmo, mov.inclX, mov.inclY, us.distCm, solo.percentagem,
                  (unsigned long)diag.loopMaxUsPublicado, (unsigned long)diag.fps);
    if (agora < 60000UL || reinicio.foiCrash) imprimirReinicio();   // repete: o monitor apanha-o
  }
}

/*
 * ============================================================================
 *  15. SETUP & LOOP
 * ============================================================================
 */
void setup() {
  analisarReinicio();                                             // antes de tudo
  Serial.begin(115200);

  preferencias.begin("mangue", false);
  brilhoGeralPct = (uint8_t)limitar(preferencias.getUChar("brilho", BRILHO_GERAL_PADRAO),
                                     BRILHO_GERAL_MIN, BRILHO_GERAL_MAX);
  uint8_t p = preferencias.getUChar("paleta", (uint8_t)Paleta::MANGUE);
  paletaAtual = p < (uint8_t)Paleta::TOTAL ? (Paleta)p : Paleta::MANGUE;
  corUnica = preferencias.getUInt("cor", COR_UNICA_PADRAO) & 0xFFFFFF;
  rot.automatica = preferencias.getUChar("autorot", AUTO_ROTACAO_PADRAO) != 0;
  rot.quartoFixo = preferencias.getUChar("rotfix", 0) & 3;
  if (!rot.automatica) { rot.quarto = rot.quartoFixo; rot.anguloExibido = rot.quartoFixo * 90.0f; }
  eixos.quarto = preferencias.getUChar("eixq", CORRECAO_EIXOS_PADRAO) & 3;
  eixos.espelho = preferencias.getUChar("eixm", ESPELHO_EIXOS_PADRAO) != 0;
  ordemCor = preferencias.getUChar("ordem", ORDEM_COR_PADRAO);
  if (ordemCor >= NUM_ORDENS_COR) ordemCor = ORDEM_COR_PADRAO;
  matriz.updateType(ORDENS_COR[ordemCor].tipo + NEO_KHZ800);
  Serial.printf("[LED] ordem de cor: %s\n", ORDENS_COR[ordemCor].nome);

  matriz.begin();          // sem setBrightness(): o brilho é calculado por pixel
  matriz.clear();
  matriz.show();

  Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, FREQ_I2C_HZ);
  Wire.setTimeOut(TIMEOUT_I2C_MS);

#if USAR_ULTRASSOM
  pinMode(PINO_US_TRIG, OUTPUT);
  digitalWrite(PINO_US_TRIG, LOW);
  pinMode(PINO_US_ECHO, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PINO_US_ECHO), isrEco, CHANGE);
#endif
  pinMode(PINO_BOTAO, INPUT_PULLUP);                              // BOOT: LOW = premido

  unsigned long agora = millis();
  { float c[3]; corEmocao(Emocao::ESPERA, c); rgbParaOklch(c, vis.corLch); }   // arranca já na cor da paleta
  vis.tModo = agora;
  vis.tInicioCrossfade = agora;
  olhos.tProximaSacada = agora + 2500;
  abrirPainel(agora, ARRANQUE_MAX_MS, -1);                        // verificação inicial
  painel.arranque = true;
  olhos.tProximoPiscar = agora + DURACAO_ACORDAR_MS + 250;

  imprimirReinicio();
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
  configurarRotas();
  if (wm.autoConnect(NOME_AP_CONFIG)) Serial.println("[REDE] Ligado com credenciais guardadas");
  else Serial.printf("[REDE] Portal aberto: ligue-se a \"%s\"\n", NOME_AP_CONFIG);
}

void loop() {
  migalha(Etapa::REDE);
  uint32_t t0 = micros();
  unsigned long agora = millis();

  gerirRede(agora);
  migalha(Etapa::BOTAO);       atualizarBotao(agora);
  migalha(Etapa::IMU);         atualizarIMU(agora);
  migalha(Etapa::ULTRASSOM);   atualizarUltrassom(agora);
  migalha(Etapa::SOLO);        atualizarSolo(agora);
  migalha(Etapa::SAUDE);       avaliarSaude(agora);
  migalha(Etapa::ENERGIA);     atualizarEnergia(agora);
  migalha(Etapa::EMOCAO);      atualizarEmocao(agora);
  migalha(Etapa::RENDER);
  if (temporizador(tFrame, INTERVALO_FRAME_MS, agora)) renderizarFrame(agora);
  if (quadroPronto) {          // show() aqui, com a pilha quase vazia (a lib usa ~6 KB)
    migalha(Etapa::LEDS);
    matriz.show();
    quadroPronto = false;
    diag.frames++;
  }
  migalha(Etapa::DIAGNOSTICO);

  registarDiagnostico(micros() - t0, agora);
}
