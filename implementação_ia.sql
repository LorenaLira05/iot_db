-- implementação_ia.sql
-- Camada de IA. Roda depois do schema.sql. Idempotente.
-- Convenções: datas em UTC (TIMESTAMP). Colunas de nível/classificação/status destas tabelas
-- continuam VARCHAR (o backend grava e compara texto). cortes_alerta_preditivo e
-- alertas_preditivos ficam no schema.sql.
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

-- previsao (schema.sql) passa a apontar para o modelo que a gerou (além do texto modelo_ultilizado)
ALTER TABLE previsao ADD COLUMN IF NOT EXISTS fk_modelo_id_modelo INTEGER
    CONSTRAINT fk_previsao_modelo REFERENCES ia_modelo (id_modelo) ON DELETE SET NULL;

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

/* ALERTA PREDITIVO */
CREATE TABLE IF NOT EXISTS alerta_preditivo (
    id_alerta_preditivo  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote      INTEGER NOT NULL,
    fk_modelo_id_modelo  INTEGER NOT NULL,
    temperatura          DOUBLE PRECISION,
    umidade              NUMERIC(5,2),
    classificacao        VARCHAR(50) NOT NULL,
    probabilidade        NUMERIC(5,2),
    motivo               TEXT,
    data_previsao        TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    status               VARCHAR(50) DEFAULT 'aberto',
    CONSTRAINT fk_alerta_preditivo_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_alerta_preditivo_modelo
        FOREIGN KEY (fk_modelo_id_modelo) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT ck_alerta_preditivo_umid CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100)
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
    nivel_risco             VARCHAR(50) NOT NULL,
    probabilidade           NUMERIC(5,2),
    justificativa           TEXT,
    data_previsao           TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_risco_lote
        FOREIGN KEY (fk_lote_id_lote) REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_risco_modelo
        FOREIGN KEY (fk_modelo_id_modelo) REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    CONSTRAINT fk_risco_previsao
        FOREIGN KEY (fk_previsao_id_previsao) REFERENCES previsao (id_previsao) ON DELETE SET NULL,
    CONSTRAINT ck_risco_periodo CHECK (periodo_fim IS NULL OR periodo_inicio IS NULL OR periodo_fim >= periodo_inicio),
    CONSTRAINT ck_risco_umid    CHECK (umidade IS NULL OR umidade BETWEEN 0 AND 100)
);

/* PARAMETRO_PREJUIZO */
CREATE TABLE IF NOT EXISTS parametro_prejuizo (
    id_parametro INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nivel_risco  VARCHAR(20) NOT NULL UNIQUE,
    fracao_perda          NUMERIC(5,4) NOT NULL,
    tolerancia_horas      NUMERIC(5,2) NOT NULL,
    acrescimo_hora        NUMERIC(5,4) NOT NULL,
    periodo_analise_dias  INTEGER NOT NULL,
    validade_preco_dias   INTEGER NOT NULL,
    justificativa         TEXT NOT NULL,
    aprovado              BOOLEAN NOT NULL DEFAULT FALSE,
    data_aprovacao        TIMESTAMP,
    responsavel_aprovacao VARCHAR(150),
    CONSTRAINT chk_parametro_risco CHECK (nivel_risco IN ('baixo', 'médio', 'alto')),
    CONSTRAINT chk_fracao_perda    CHECK (fracao_perda BETWEEN 0 AND 1)
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
CREATE INDEX IF NOT EXISTS idx_prejuizo_lote             ON previsao_prejuizo (fk_lote_id_lote, data_previsao DESC);
CREATE INDEX IF NOT EXISTS idx_irrigacao_lote            ON previsao_irrigacao (fk_lote_id_lote, data_previsao DESC);
CREATE INDEX IF NOT EXISTS idx_risco_lote                ON risco_climatico (fk_lote_id_lote, data_previsao DESC);

/* ===== PARECER AGRONÔMICO (rascunho gerado por IA, editado e assinado pelo responsável técnico) =====
   Usado por src/api/routers/pareceres.py. Depois de assinado, o parecer não pode mais ser alterado. */
CREATE TABLE IF NOT EXISTS parecer_agronomico (
    id_parecer          INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote     INTEGER      NOT NULL REFERENCES lote (id_lote) ON DELETE RESTRICT,
    tipo                VARCHAR(20)  NOT NULL,
    status              VARCHAR(20)  NOT NULL DEFAULT 'rascunho',
    texto_gerado        TEXT         NOT NULL,
    texto_final         TEXT         NOT NULL,
    dados_entrada       JSONB        NOT NULL,
    hash_entrada        VARCHAR(64)  NOT NULL,
    origem_texto        VARCHAR(20)  NOT NULL,
    provedor_modelo     VARCHAR(100),
    fk_modelo_id_modelo INTEGER      REFERENCES ia_modelo (id_modelo) ON DELETE RESTRICT,
    fk_gerado_por       INTEGER      REFERENCES usuario (id_usuario) ON DELETE RESTRICT,
    gerado_em           TIMESTAMP    NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    assinado_em         TIMESTAMP,
    assinante_nome      VARCHAR(150),
    crea                VARCHAR(40),
    fk_assinado_por     INTEGER      REFERENCES usuario (id_usuario) ON DELETE RESTRICT,
    CONSTRAINT ck_parecer_tipo   CHECK (tipo IN ('agronomico', 'fisiologico')),
    CONSTRAINT ck_parecer_status CHECK (status IN ('rascunho', 'assinado')),
    CONSTRAINT ck_parecer_origem CHECK (origem_texto IN ('llm', 'template')),
    CONSTRAINT ck_parecer_assinatura CHECK (
        status <> 'assinado'
        OR (assinado_em IS NOT NULL AND assinante_nome IS NOT NULL AND crea IS NOT NULL))
);
CREATE INDEX IF NOT EXISTS idx_parecer_agro_lote ON parecer_agronomico (fk_lote_id_lote, gerado_em DESC);

CREATE OR REPLACE FUNCTION fn_parecer_travar_assinado()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status = 'assinado' THEN
        RAISE EXCEPTION 'Parecer % já está assinado e não pode ser alterado.', OLD.id_parecer;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_parecer_travar_assinado ON parecer_agronomico;
CREATE TRIGGER trg_parecer_travar_assinado
BEFORE UPDATE ON parecer_agronomico
FOR EACH ROW EXECUTE FUNCTION fn_parecer_travar_assinado();
