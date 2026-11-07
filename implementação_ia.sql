-- implementação_ia.sql
-- Camada de IA. Roda depois do schema.sql. Idempotente.
-- Convenções: nível/risco/severidade = enum nivel_alerta ('baixo','médio','alto');
--             probabilidade sempre de 0 a 1; datas em UTC (TIMESTAMP).
-- Histórico de previsões não é apagado junto com o lote (ON DELETE RESTRICT).

/* IA MODELO */
CREATE TABLE IF NOT EXISTS ia_modelo (
    id_modelo        INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome             VARCHAR(150) NOT NULL,
    tipo_modelo      VARCHAR(100) NOT NULL,
    finalidade       VARCHAR(150) NOT NULL,
    algoritmo        VARCHAR(150),
    versao           VARCHAR(50),
    status           VARCHAR(50)  NOT NULL,
    data_criacao     TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    data_atualizacao TIMESTAMP
);

-- previsao (schema.sql) passa a apontar para o modelo que a gerou (além do texto modelo_utilizado)
ALTER TABLE previsao ADD COLUMN IF NOT EXISTS fk_modelo_id_modelo INTEGER
    CONSTRAINT fk_previsao_modelo REFERENCES ia_modelo (id_modelo) ON DELETE SET NULL;

/* CORTES DO ALERTA PREDITIVO
   Nível = o corte de maior limiar_min que a probabilidade alcança.
   limiar_min é INCLUSIVO e limiar_max é EXCLUSIVO (exceto o último, 1.0), então não há
   lacuna entre faixas. Probabilidade abaixo do menor limiar_min = sem alerta. */
CREATE TABLE IF NOT EXISTS cortes_alerta_preditivo (
    id_corte         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nivel            nivel_alerta  NOT NULL UNIQUE,
    limiar_min       NUMERIC(5, 4) NOT NULL,
    limiar_max       NUMERIC(5, 4) NOT NULL,
    acao_recomendada TEXT          NOT NULL,
    data_atualizacao TIMESTAMP     NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT ck_corte_limiar CHECK (limiar_min >= 0 AND limiar_max <= 1 AND limiar_min < limiar_max)
);

/* CHAT CONVERSA */
CREATE TABLE IF NOT EXISTS chat_conversa (
    id_conversa           INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_usuario_id_usuario INTEGER NOT NULL,
    titulo                VARCHAR(150),
    data_inicio           TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    data_ultima_mensagem  TIMESTAMP,
    CONSTRAINT fk_chat_conversa_usuario
        FOREIGN KEY (fk_usuario_id_usuario)
        REFERENCES usuario (id_usuario)
        ON DELETE CASCADE
);

/* CHAT MENSAGEM */
CREATE TABLE IF NOT EXISTS chat_mensagem (
    id_mensagem             INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_conversa_id_conversa INTEGER NOT NULL,
    tipo                    VARCHAR(20) NOT NULL,
    mensagem                TEXT NOT NULL,
    data_hora               TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    modelo_utilizado        VARCHAR(150),
    CONSTRAINT fk_chat_mensagem_conversa
        FOREIGN KEY (fk_conversa_id_conversa)
        REFERENCES chat_conversa (id_conversa)
        ON DELETE CASCADE
);

/* PREVISAO PREJUIZO (detalhe tipado de previsao.tipo = 'prejuizo') */
CREATE TABLE IF NOT EXISTS previsao_prejuizo (
    id_previsao_prejuizo INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote      INTEGER NOT NULL,
    fk_modelo_id_modelo  INTEGER NOT NULL,
    fk_previsao_id_previsao INTEGER,
    temperatura          DOUBLE PRECISION,
    umidade              NUMERIC(5,2),
    periodo_inicio       DATE,
    periodo_fim          DATE,
    prejuizo_estimado    NUMERIC(12,2),
    data_previsao        TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_prejuizo_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_prejuizo_modelo
        FOREIGN KEY (fk_modelo_id_modelo) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_prejuizo_previsao
        FOREIGN KEY (fk_previsao_id_previsao) REFERENCES previsao (id_previsao) ON DELETE SET NULL,
    CONSTRAINT ck_prejuizo_periodo CHECK (periodo_fim IS NULL OR periodo_inicio IS NULL OR periodo_fim >= periodo_inicio),
    CONSTRAINT ck_prejuizo_umidade CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100)
);

/* ALERTA PREDITIVO (substitui a antiga alertas_preditivos do schema.sql) */
CREATE TABLE IF NOT EXISTS alerta_preditivo (
    id_alerta_preditivo  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote      INTEGER NOT NULL,
    fk_modelo_id_modelo  INTEGER NOT NULL,
    fk_sensor_id_sensor  INTEGER,
    temperatura          DOUBLE PRECISION,
    umidade              NUMERIC(5,2),
    classificacao        nivel_alerta NOT NULL,
    probabilidade        NUMERIC(5,4),
    motivo               TEXT,
    previsao_data_hora   TIMESTAMP,       -- instante para o qual o risco foi previsto (UTC)
    data_previsao        TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    status               VARCHAR(50) NOT NULL DEFAULT 'aberto',
    CONSTRAINT fk_alerta_preditivo_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_alerta_preditivo_modelo
        FOREIGN KEY (fk_modelo_id_modelo) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_alerta_preditivo_sensor
        FOREIGN KEY (fk_sensor_id_sensor) REFERENCES sensor (id_sensor) ON DELETE SET NULL,
    CONSTRAINT ck_alerta_preditivo_prob   CHECK (probabilidade IS NULL OR probabilidade BETWEEN 0 AND 1),
    CONSTRAINT ck_alerta_preditivo_umid   CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100),
    CONSTRAINT ck_alerta_preditivo_status CHECK (status IN ('aberto','reconhecido','resolvido'))
);

/* PREVISAO IRRIGAÇÃO (detalhe tipado de previsao.tipo = 'irrigacao') */
CREATE TABLE IF NOT EXISTS previsao_irrigacao (
    id_previsao_irrigacao   INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote         INTEGER NOT NULL,
    fk_modelo_classificacao INTEGER NOT NULL,
    fk_modelo_regressao     INTEGER,
    fk_previsao_id_previsao INTEGER,
    temperatura             DOUBLE PRECISION,
    umidade                 NUMERIC(5,2),
    necessita_irrigacao     BOOLEAN NOT NULL,
    quantidade_agua_mm      NUMERIC(8,2),
    data_previsao           TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_irrigacao_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_irrigacao_modelo_classificacao
        FOREIGN KEY (fk_modelo_classificacao) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_irrigacao_modelo_regressao
        FOREIGN KEY (fk_modelo_regressao) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_irrigacao_previsao
        FOREIGN KEY (fk_previsao_id_previsao) REFERENCES previsao (id_previsao) ON DELETE SET NULL,
    CONSTRAINT ck_irrigacao_agua  CHECK (quantidade_agua_mm IS NULL OR quantidade_agua_mm >= 0),
    CONSTRAINT ck_irrigacao_umid  CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100)
);

/* RISCO CLIMATICO (detalhe tipado de previsao.tipo = 'risco_climatico') */
CREATE TABLE IF NOT EXISTS risco_climatico (
    id_risco_climatico      INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote         INTEGER NOT NULL,
    fk_modelo_id_modelo     INTEGER NOT NULL,
    fk_previsao_id_previsao INTEGER,
    temperatura             DOUBLE PRECISION,
    umidade                 NUMERIC(5,2),
    periodo_inicio          DATE,
    periodo_fim             DATE,
    nivel_risco             nivel_alerta NOT NULL,
    probabilidade           NUMERIC(5,4),
    justificativa           TEXT,
    data_previsao           TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_risco_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_risco_modelo
        FOREIGN KEY (fk_modelo_id_modelo) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_risco_previsao
        FOREIGN KEY (fk_previsao_id_previsao) REFERENCES previsao (id_previsao) ON DELETE SET NULL,
    CONSTRAINT ck_risco_prob    CHECK (probabilidade IS NULL OR probabilidade BETWEEN 0 AND 1),
    CONSTRAINT ck_risco_periodo CHECK (periodo_fim IS NULL OR periodo_inicio IS NULL OR periodo_fim >= periodo_inicio),
    CONSTRAINT ck_risco_umid    CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100)
);

/* PARAMETRO_PREJUIZO */
CREATE TABLE IF NOT EXISTS parametro_prejuizo (
    id_parametro INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nivel_risco  nivel_alerta NOT NULL UNIQUE,
    fracao_perda          NUMERIC(5,4) NOT NULL,
    tolerancia_horas      NUMERIC(5,2) NOT NULL,
    acrescimo_hora        NUMERIC(5,4) NOT NULL,
    periodo_analise_dias  INTEGER NOT NULL,
    validade_preco_dias   INTEGER NOT NULL,
    justificativa         TEXT NOT NULL,
    aprovado              BOOLEAN NOT NULL DEFAULT FALSE,
    data_aprovacao        TIMESTAMP,
    responsavel_aprovacao VARCHAR(150),
    CONSTRAINT chk_fracao_perda CHECK (fracao_perda BETWEEN 0 AND 1)
);

/* PARAMETRO_IRRIGACAO */
CREATE TABLE IF NOT EXISTS parametro_irrigacao (
    id_parametro INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta INTEGER NOT NULL UNIQUE REFERENCES fruta (id_fruta),
    kc                         NUMERIC(5,3) NOT NULL,
    fase_cultura               VARCHAR(80)  NOT NULL,
    chuva_efetiva_percentual   NUMERIC(5,2) NOT NULL,
    probabilidade_chuva_minima NUMERIC(5,2) NOT NULL,
    irrigacao_minima_mm        NUMERIC(6,2) NOT NULL,
    vpd_limite_kpa             NUMERIC(5,2) NOT NULL,
    umidade_solo_minima        NUMERIC(5,2) NOT NULL,
    justificativa              TEXT NOT NULL,
    aprovado                   BOOLEAN NOT NULL DEFAULT FALSE,
    data_aprovacao             TIMESTAMP,
    responsavel_aprovacao      VARCHAR(150),
    CONSTRAINT chk_kc                  CHECK (kc > 0),
    CONSTRAINT chk_chuva_efetiva       CHECK (chuva_efetiva_percentual BETWEEN 0 AND 100),
    CONSTRAINT chk_probabilidade_chuva CHECK (probabilidade_chuva_minima BETWEEN 0 AND 100),
    CONSTRAINT chk_irrigacao_minima    CHECK (irrigacao_minima_mm >= 0),
    CONSTRAINT chk_vpd                 CHECK (vpd_limite_kpa > 0),
    CONSTRAINT chk_umidade_solo        CHECK (umidade_solo_minima BETWEEN 0 AND 100)
);

/* PARECER IA: cache do parecer por lote (TIMESTAMPTZ de propósito: expiração do cache) */
CREATE TABLE IF NOT EXISTS parecer_ia (
    id               INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    id_lote          INTEGER NOT NULL REFERENCES lote (id_lote) ON DELETE CASCADE,
    modelo           VARCHAR(100) NOT NULL,
    snapshot_entrada JSONB NOT NULL,
    conteudo_json    JSONB NOT NULL,
    data_geracao     TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
    data_expiracao   TIMESTAMP WITH TIME ZONE NOT NULL
);

/* ===== ÍNDICES ===== */
CREATE INDEX IF NOT EXISTS idx_parecer_ia_lote_expiracao ON parecer_ia (id_lote, data_expiracao DESC);
CREATE INDEX IF NOT EXISTS idx_chat_conversa_usuario     ON chat_conversa (fk_usuario_id_usuario, data_ultima_mensagem DESC);
CREATE INDEX IF NOT EXISTS idx_chat_mensagem_conversa    ON chat_mensagem (fk_conversa_id_conversa, data_hora);
CREATE INDEX IF NOT EXISTS idx_alerta_preditivo_lote     ON alerta_preditivo (fk_lote_id_lote, status, data_previsao DESC);
CREATE INDEX IF NOT EXISTS idx_alerta_preditivo_sensor   ON alerta_preditivo (fk_sensor_id_sensor);
CREATE INDEX IF NOT EXISTS idx_prejuizo_lote             ON previsao_prejuizo (fk_lote_id_lote, data_previsao DESC);
CREATE INDEX IF NOT EXISTS idx_irrigacao_lote            ON previsao_irrigacao (fk_lote_id_lote, data_previsao DESC);
CREATE INDEX IF NOT EXISTS idx_risco_lote                ON risco_climatico (fk_lote_id_lote, data_previsao DESC);
