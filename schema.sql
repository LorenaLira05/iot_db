/* =========================
   TIPOS ENUM
   ========================= */

CREATE TYPE status_usuario AS ENUM ('ativo', 'inativo');

CREATE TYPE status_sensor AS ENUM ('ativo', 'inativo');

CREATE TYPE nivel_alerta AS ENUM ('baixo', 'médio', 'alto');

CREATE TYPE status_lote AS ENUM (
    'Em produção',
    'Pronto para colheita',
    'Colhido',
    'Em transporte',
    'Armazenado',
    'Finalizado'
);


/* =========================
   PERFIL
   ========================= */

CREATE TABLE perfil (
    id_perfil INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(150),
    descricao VARCHAR(150)
);


/* =========================
   USUARIO
   ========================= */

CREATE TABLE usuario (
    id_usuario INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(150),
    email VARCHAR(150),
    senha VARCHAR(150),
    status status_usuario,
    data_cadastro DATE,
    fk_perfil_id_perfil INTEGER,

    CONSTRAINT fk_usuario_perfil
        FOREIGN KEY (fk_perfil_id_perfil)
        REFERENCES perfil (id_perfil)
        ON DELETE RESTRICT
);


/* =========================
   FRUTA
   ========================= */

CREATE TABLE fruta (
    id_fruta INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(150)
);


/* =========================
   LOTE
   ========================= */

CREATE TABLE lote (
    id_lote INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    data_colheita DATE,
    quantidade INTEGER,
    status status_lote,
    destino VARCHAR(150),
    fk_fruta_id_fruta INTEGER,

    CONSTRAINT fk_lote_fruta
        FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta)
        ON DELETE CASCADE
);


/* =========================
   SENSOR
   ========================= */

CREATE TABLE sensor (
    id_sensor INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(150),
    tipo_sensor VARCHAR(150),
    localizacao VARCHAR(150),
    channel_id INTEGER,
    status status_sensor,
    fk_lote_id_lote INTEGER,

    CONSTRAINT fk_sensor_lote
        FOREIGN KEY (fk_lote_id_lote)
        REFERENCES lote (id_lote)
        ON DELETE SET NULL
);


/* =========================
   LEITURA CLIMATICA
   ========================= */

CREATE TABLE leitura_climatica (
    id_leitura INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    temperatura DOUBLE PRECISION,
    umidade NUMERIC(5, 2),
    data_hora TIMESTAMP,
    data_recebimento TIMESTAMP,
    origem VARCHAR(150),
    entry_id_thingspeak INTEGER,
    fk_sensor_id_sensor INTEGER,

    CONSTRAINT fk_leitura_climatica_sensor
        FOREIGN KEY (fk_sensor_id_sensor)
        REFERENCES sensor (id_sensor)
        ON DELETE RESTRICT,

    CONSTRAINT uq_leitura_sensor_entry
        UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak)
);

CREATE INDEX idx_leitura_sensor_data
    ON leitura_climatica (fk_sensor_id_sensor, data_hora);

/* =========================
   CONFIG PARAMETRO
   ========================= */

CREATE TABLE config_parametro (
    id_config INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta INTEGER,
    temp_min DOUBLE PRECISION,
    temp_max DOUBLE PRECISION,
    umidade_min NUMERIC(5, 2),
    umidade_max NUMERIC(5, 2),

    CONSTRAINT fk_config_fruta
        FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta)
        ON DELETE CASCADE
);


/* =========================
   DADOS MERCADO
   ========================= */

CREATE TABLE dados_mercado (
    id_mercado INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta INTEGER,
    preco DOUBLE PRECISION,
    demanda_exportacao NUMERIC(5, 2),
    data_refencia DATE,

    CONSTRAINT fk_dados_mercado_fruta
        FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta)
        ON DELETE CASCADE
);


/* =========================
   PREVISAO
   ========================= */

CREATE TABLE previsao (
    id_previsao INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote INTEGER,
    data DATE,
    inicio TIME,
    fim TIME,
    probabilidade NUMERIC(5, 2),
    resultado VARCHAR(150),
    modelo_ultilizado VARCHAR(150),

    CONSTRAINT fk_previsao_lote
        FOREIGN KEY (fk_lote_id_lote)
        REFERENCES lote (id_lote)
        ON DELETE CASCADE
);


/* =========================
   PREVISAO CLIMATICA (associativa)
   ========================= */

CREATE TABLE previsao_climatica (
    fk_previsao_id_previsao INTEGER,
    fk_leitura_climatica_id_leitura INTEGER,

    CONSTRAINT fk_previsao_climatica_previsao
        FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao)
        ON DELETE RESTRICT,

    CONSTRAINT fk_previsao_climatica_leitura
        FOREIGN KEY (fk_leitura_climatica_id_leitura)
        REFERENCES leitura_climatica (id_leitura)
        ON DELETE SET NULL
);


/* =========================
   PREVISAO MERCADO (associativa)
   ========================= */

CREATE TABLE previsao_mercado (
    fk_dados_mercado_id_mercado INTEGER,
    fk_previsao_id_previsao INTEGER,

    CONSTRAINT fk_previsao_mercado_dados
        FOREIGN KEY (fk_dados_mercado_id_mercado)
        REFERENCES dados_mercado (id_mercado)
        ON DELETE RESTRICT,

    CONSTRAINT fk_previsao_mercado_previsao
        FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao)
        ON DELETE SET NULL
);


/* =========================
   ALERTA
   ========================= */

CREATE TABLE alerta (
    id_alerta INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    tipo_alerta VARCHAR(150),
    descricao VARCHAR(150),
    nivel nivel_alerta,
    hora TIME,
    status VARCHAR(150),
    fk_previsao_id_previsao INTEGER,
    fk_leitura_id_leitura INTEGER,

    CONSTRAINT fk_alerta_previsao
        FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao)
        ON DELETE CASCADE,

    CONSTRAINT fk_alerta_leitura
        FOREIGN KEY (fk_leitura_id_leitura)
        REFERENCES leitura_climatica (id_leitura)
        ON DELETE CASCADE
);


/* =========================
   LOG ACESSO
   ========================= */

CREATE TABLE log_acesso (
    id_log INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    data_hora TIMESTAMP,
    acao VARCHAR(150),
    ip INTEGER,
    resultado VARCHAR(150),
    fk_usuario_id_usuario INTEGER,

    CONSTRAINT fk_log_acesso_usuario
        FOREIGN KEY (fk_usuario_id_usuario)
        REFERENCES usuario (id_usuario)
        ON DELETE CASCADE
);


/* =========================
   RELATORIO
   ========================= */

CREATE TABLE relatorio (
    id_relatorio INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    tipo VARCHAR(150),
    descricao VARCHAR(150),
    data_geracao DATE,
    inicio DATE,
    fim DATE,
    fk_usuario_id_usuario INTEGER,

    CONSTRAINT fk_relatorio_usuario
        FOREIGN KEY (fk_usuario_id_usuario)
        REFERENCES usuario (id_usuario)
        ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS leitura_rejeitada (
    id_rejeicao         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_sensor_id_sensor INTEGER,
    entry_id_thingspeak INTEGER,
    temperatura_bruta   VARCHAR(100),
    umidade_bruta       VARCHAR(100),
    data_hora_bruta     VARCHAR(100),
    motivo              VARCHAR(255) NOT NULL,
    origem              VARCHAR(150),
    data_rejeicao       TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),

    CONSTRAINT fk_leitura_rejeitada_sensor
        FOREIGN KEY (fk_sensor_id_sensor)
        REFERENCES sensor (id_sensor)
        ON DELETE SET NULL,

    CONSTRAINT uq_rejeitada_sensor_entry
        UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak)
);

CREATE INDEX IF NOT EXISTS idx_rejeitada_sensor_data
    ON leitura_rejeitada (fk_sensor_id_sensor, data_rejeicao);

DO $$
BEGIN
    IF (SELECT data_type
          FROM information_schema.columns
         WHERE table_name = 'log_acesso' AND column_name = 'ip') = 'integer' THEN
        ALTER TABLE log_acesso ALTER COLUMN ip TYPE INET USING NULL;
    END IF;
END $$;

CREATE INDEX IF NOT EXISTS idx_log_acesso_data ON log_acesso (data_hora);

COMMIT;

ALTER TABLE sensor
  ADD COLUMN IF NOT EXISTS ambiente VARCHAR(10) NOT NULL DEFAULT 'ar',
  ADD CONSTRAINT ck_sensor_ambiente CHECK (ambiente IN ('ar','solo'));

ALTER TABLE config_parametro
  ADD COLUMN IF NOT EXISTS etapa VARCHAR(15) NOT NULL DEFAULT 'armazenamento',
  ADD CONSTRAINT ck_config_etapa CHECK (etapa IN ('campo','armazenamento','transporte'));

ALTER TABLE config_parametro
  ADD CONSTRAINT uq_config_fruta_etapa UNIQUE (fk_fruta_id_fruta, etapa);

ALTER TABLE alerta ADD COLUMN IF NOT EXISTS data_hora TIMESTAMP;
UPDATE alerta SET data_hora = CURRENT_DATE + hora WHERE data_hora IS NULL;  -- legado: assume hoje
ALTER TABLE alerta ALTER COLUMN data_hora SET DEFAULT now();
-- Depois de atualizar o trigger/backend, rode:
--   ALTER TABLE alerta ALTER COLUMN data_hora SET NOT NULL;
--   ALTER TABLE alerta DROP COLUMN hora;
CREATE INDEX IF NOT EXISTS idx_alerta_data_hora ON alerta (data_hora);

ALTER TABLE previsao
  ADD COLUMN IF NOT EXISTS tipo      VARCHAR(20),
  ADD COLUMN IF NOT EXISTS categoria nivel_alerta,          -- reaproveita o enum (baixo/médio/alto)
  ADD COLUMN IF NOT EXISTS valor     NUMERIC(14,2),
  ADD COLUMN IF NOT EXISTS unidade   VARCHAR(15),           -- 'mm', 'L', 'BRL'
  ADD COLUMN IF NOT EXISTS gerado_em TIMESTAMP NOT NULL DEFAULT now(),
  ADD COLUMN IF NOT EXISTS detalhes  JSONB,                 -- insumos + explicação
  ADD CONSTRAINT ck_previsao_tipo CHECK (
    tipo IS NULL OR tipo IN ('risco_climatico','irrigacao','prejuizo','janela_colheita'));
CREATE INDEX IF NOT EXISTS idx_previsao_lote_tipo ON previsao (fk_lote_id_lote, tipo, gerado_em DESC);

CREATE TABLE IF NOT EXISTS porto (
  id_porto  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  nome      VARCHAR(80) NOT NULL UNIQUE,
  uf        CHAR(2)     NOT NULL,
  latitude  NUMERIC(9,6) NOT NULL,
  longitude NUMERIC(9,6) NOT NULL
);

CREATE TABLE IF NOT EXISTS rota (
  id_rota           INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  fk_porto_id_porto INTEGER NOT NULL UNIQUE REFERENCES porto (id_porto) ON DELETE CASCADE,
  origem_nome       VARCHAR(80) NOT NULL DEFAULT 'Petrolina',
  distancia_km      NUMERIC(8,1),
  duracao_min       INTEGER,
  tracado           JSONB,                 -- GeoJSON da API de rotas
  atualizado_em     TIMESTAMP NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS viagem (
  id_viagem         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  fk_lote_id_lote   INTEGER NOT NULL REFERENCES lote (id_lote)  ON DELETE CASCADE,
  fk_porto_id_porto INTEGER NOT NULL REFERENCES porto (id_porto) ON DELETE RESTRICT,
  saida             TIMESTAMP NOT NULL,
  chegada_prevista  TIMESTAMP,
  status            VARCHAR(15) NOT NULL DEFAULT 'programada'
    CHECK (status IN ('programada','em_transporte','concluida','cancelada'))
);
CREATE INDEX IF NOT EXISTS idx_viagem_status ON viagem (status);

INSERT INTO porto (nome, uf, latitude, longitude) VALUES   -- coordenadas aproximadas, confira
  ('Pecém',    'CE',  -3.5400, -38.8000),
  ('Suape',    'PE',  -8.3900, -34.9600),
  ('Salvador', 'BA', -12.9700, -38.5100),
  ('Natal',    'RN',  -5.7800, -35.2000)
ON CONFLICT (nome) DO NOTHING;

CREATE TABLE IF NOT EXISTS perda_risco (
  categoria    nivel_alerta PRIMARY KEY,
  fracao_perda NUMERIC(4,3) NOT NULL CHECK (fracao_perda BETWEEN 0 AND 1)
);
INSERT INTO perda_risco VALUES ('baixo',0.02),('médio',0.10),('alto',0.30)
ON CONFLICT (categoria) DO NOTHING;

ALTER TABLE lote
  ADD COLUMN IF NOT EXISTS unidade VARCHAR(10) NOT NULL DEFAULT 'kg',
  ADD CONSTRAINT ck_lote_unidade CHECK (unidade IN ('kg','caixa','t'));
ALTER TABLE dados_mercado
  ADD COLUMN IF NOT EXISTS unidade VARCHAR(10) NOT NULL DEFAULT 'kg',
  ADD COLUMN IF NOT EXISTS moeda   CHAR(3)     NOT NULL DEFAULT 'BRL';

CREATE OR REPLACE VIEW vw_leitura_hora AS
SELECT fk_sensor_id_sensor,
       date_trunc('hour', data_hora) AS hora,
       ROUND(AVG(temperatura)::numeric, 2) AS temperatura_media,
       ROUND(AVG(umidade), 2)              AS umidade_media,
       COUNT(*)                            AS n_leituras
FROM leitura_climatica
GROUP BY fk_sensor_id_sensor, date_trunc('hour', data_hora);

/* 9. selo do lote: % de leituras das últimas 24 h dentro da faixa */
CREATE OR REPLACE VIEW vw_lote_saude AS
WITH lote_etapa AS (
  SELECT l.id_lote, l.fk_fruta_id_fruta, l.status,
         CASE l.status
           WHEN 'Em produção'          THEN 'campo'
           WHEN 'Pronto para colheita' THEN 'campo'
           WHEN 'Em transporte'        THEN 'transporte'
           ELSE 'armazenamento'
         END AS etapa
  FROM lote l
), pct AS (
  SELECT le.id_lote,
         COUNT(*) AS n,
         100.0 * COUNT(*) FILTER (
           WHERE r.temperatura BETWEEN c.temp_min AND c.temp_max
             AND r.umidade     BETWEEN c.umidade_min AND c.umidade_max
         ) / NULLIF(COUNT(*), 0) AS pct_na_faixa
  FROM lote_etapa le
  JOIN sensor s ON s.fk_lote_id_lote = le.id_lote
  JOIN leitura_climatica r ON r.fk_sensor_id_sensor = s.id_sensor
                          AND r.data_hora >= now() - interval '24 hours'
  JOIN config_parametro c ON c.fk_fruta_id_fruta = le.fk_fruta_id_fruta
                         AND c.etapa = le.etapa
  GROUP BY le.id_lote
)
SELECT id_lote, n AS n_leituras, ROUND(pct_na_faixa, 1) AS pct_na_faixa,
       CASE WHEN pct_na_faixa >= 90 THEN 'Saudável'   -- limites provisórios: o grupo define
            WHEN pct_na_faixa >= 70 THEN 'Atenção'
            ELSE 'Crítico' END AS selo
FROM pct;
