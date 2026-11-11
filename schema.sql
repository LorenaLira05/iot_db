-- schema.sql
-- Convenção de datas: TIMESTAMP em UTC (o front converte para America/Recife).
-- Exceção: auth.sql e parecer_ia usam TIMESTAMPTZ (o backend manipula expiração de token com fuso).

/* ===== TIPOS ENUM ===== */
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'status_usuario') THEN
        CREATE TYPE status_usuario AS ENUM ('ativo', 'inativo');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'status_sensor') THEN
        CREATE TYPE status_sensor AS ENUM ('ativo', 'inativo');
    END IF;
    -- nivel_alerta: nível de alerta, de previsão e de perda por risco
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'nivel_alerta') THEN
        CREATE TYPE nivel_alerta AS ENUM ('baixo', 'médio', 'alto');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'status_lote') THEN
        CREATE TYPE status_lote AS ENUM (
            'Em produção', 'Pronto para colheita', 'Colhido',
            'Em transporte', 'Armazenado', 'Finalizado');
    END IF;
END $$;

/* ===== TABELAS ===== */
CREATE TABLE IF NOT EXISTS perfil (
    id_perfil INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome      VARCHAR(150),
    descricao VARCHAR(150)
);

CREATE TABLE IF NOT EXISTS usuario (
    id_usuario          INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome                VARCHAR(150),
    email               VARCHAR(150),
    senha               VARCHAR(150),
    status              status_usuario,
    data_cadastro       DATE,
    fk_perfil_id_perfil INTEGER,
    CONSTRAINT fk_usuario_perfil FOREIGN KEY (fk_perfil_id_perfil)
        REFERENCES perfil (id_perfil) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS fruta (
    id_fruta INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome     VARCHAR(150)
);

CREATE TABLE IF NOT EXISTS propriedade (
    id_propriedade INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(150) NOT NULL,
    codigo VARCHAR(30) UNIQUE,
    cidade VARCHAR(80), uf CHAR(2), setor VARCHAR(40),
    latitude NUMERIC(9,6), longitude NUMERIC(9,6),
    area_ha NUMERIC(10,2),
    tipo_solo VARCHAR(150),
    moeda_exibicao CHAR(3) NOT NULL DEFAULT 'BRL'
);

CREATE TABLE IF NOT EXISTS safra (
    id_safra INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome VARCHAR(20) NOT NULL UNIQUE,          -- '2025/2026'
    data_inicio DATE NOT NULL,
    data_fim DATE,
    meta_ton_ha NUMERIC(6,2)
);

CREATE TABLE IF NOT EXISTS variedade (
    id_variedade INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta INTEGER NOT NULL REFERENCES fruta (id_fruta) ON DELETE CASCADE,
    nome VARCHAR(80) NOT NULL,
    UNIQUE (fk_fruta_id_fruta, nome)
);

CREATE TABLE IF NOT EXISTS padrao_qualidade (
    fk_variedade_id_variedade INTEGER PRIMARY KEY REFERENCES variedade (id_variedade) ON DELETE CASCADE,
    brix_min NUMERIC(4,1),
    calibre_min VARCHAR(20),
    classe VARCHAR(30)            -- ex.: 'Exportação A+'
);

-- Histórico não é apagado em cascata: lote só é removido se não tiver sensores com leitura,
-- previsões, viagens etc. (ON DELETE RESTRICT). Para encerrar um lote use status = 'Finalizado'.
CREATE TABLE IF NOT EXISTS lote (
    id_lote                       INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    data_colheita                 DATE,
    quantidade                    INTEGER,
    unidade                       VARCHAR(10) NOT NULL DEFAULT 'kg',
    peso_caixa_kg                 NUMERIC(6,2),     -- opcional; usado para converter 'caixa' em kg na conferência
    status                        status_lote,
    destino                       VARCHAR(150),
    fk_fruta_id_fruta             INTEGER,
    fk_variedade_id_variedade     INTEGER,
    fk_safra_id_safra             INTEGER,
    fk_propriedade_id_propriedade INTEGER,
    CONSTRAINT fk_lote_fruta FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta) ON DELETE RESTRICT,
    CONSTRAINT fk_lote_variedade FOREIGN KEY (fk_variedade_id_variedade)
        REFERENCES variedade (id_variedade) ON DELETE RESTRICT,
    CONSTRAINT fk_lote_safra FOREIGN KEY (fk_safra_id_safra)
        REFERENCES safra (id_safra) ON DELETE RESTRICT,
    CONSTRAINT fk_lote_propriedade FOREIGN KEY (fk_propriedade_id_propriedade)
        REFERENCES propriedade (id_propriedade) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS lote_classificacao (
    fk_lote_id_lote INTEGER REFERENCES lote (id_lote) ON DELETE CASCADE,
    categoria VARCHAR(20) NOT NULL,   -- exportacao, mercado_interno, processamento
    quantidade_kg NUMERIC(12,2) NOT NULL,
    PRIMARY KEY (fk_lote_id_lote, categoria)
);

CREATE TABLE IF NOT EXISTS sensor (
    id_sensor       INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome            VARCHAR(150),
    tipo_sensor     VARCHAR(150),
    localizacao     VARCHAR(150),
    channel_id      INTEGER,
    ambiente        VARCHAR(10) NOT NULL DEFAULT 'ar',   -- 'ar' | 'solo': diz o que temperatura/umidade deste sensor medem
    status          status_sensor,
    fk_lote_id_lote INTEGER,
    CONSTRAINT fk_sensor_lote FOREIGN KEY (fk_lote_id_lote)
        REFERENCES lote (id_lote) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS leitura_climatica (
    id_leitura          INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    temperatura         DOUBLE PRECISION,   -- sensor de ar
    umidade             NUMERIC(5, 2),      -- sensor de ar (umidade relativa, %)
    umidade_solo        NUMERIC(5, 2),      -- sensor de solo (umidade do solo, %). Não existe temperatura do solo.
    data_hora           TIMESTAMP,          -- UTC
    data_recebimento    TIMESTAMP,          -- UTC
    origem              VARCHAR(150),
    entry_id_thingspeak INTEGER,
    fk_sensor_id_sensor INTEGER,
    CONSTRAINT fk_leitura_climatica_sensor FOREIGN KEY (fk_sensor_id_sensor)
        REFERENCES sensor (id_sensor) ON DELETE RESTRICT,
    CONSTRAINT uq_leitura_sensor_entry UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak)
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
    CONSTRAINT fk_leitura_rejeitada_sensor FOREIGN KEY (fk_sensor_id_sensor)
        REFERENCES sensor (id_sensor) ON DELETE SET NULL,
    CONSTRAINT uq_rejeitada_sensor_entry UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak)
);

CREATE TABLE IF NOT EXISTS config_parametro (
    id_config         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta INTEGER,
    etapa             VARCHAR(15) NOT NULL DEFAULT 'armazenamento',
    temp_min          DOUBLE PRECISION,
    temp_max          DOUBLE PRECISION,
    umidade_min       NUMERIC(5, 2),
    umidade_max       NUMERIC(5, 2),
    CONSTRAINT fk_config_fruta FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS dados_mercado (
    id_mercado         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_fruta_id_fruta  INTEGER,
    preco              DOUBLE PRECISION,
    unidade            VARCHAR(10) NOT NULL DEFAULT 'kg',
    moeda              CHAR(3)     NOT NULL DEFAULT 'BRL',
    demanda_exportacao NUMERIC(5, 2),
    data_refencia      DATE,   -- (sic) nome mantido: o backend usa
    CONSTRAINT fk_dados_mercado_fruta FOREIGN KEY (fk_fruta_id_fruta)
        REFERENCES fruta (id_fruta) ON DELETE CASCADE
);

-- Previsão consolidada: 1 linha por previsão gerada, é ela que alimenta alerta, mercado e clima.
-- As tabelas de implementação_ia.sql (previsao_prejuizo, previsao_irrigacao, risco_climatico)
-- guardam o detalhe tipado de cada modelo e apontam para cá por fk_previsao_id_previsao.
CREATE TABLE IF NOT EXISTS previsao (
    id_previsao       INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote   INTEGER,
    data              DATE,
    inicio            TIME,
    fim               TIME,
    probabilidade     NUMERIC(5, 2),
    resultado         VARCHAR(150),
    modelo_ultilizado VARCHAR(150),   -- (sic) nome mantido: o backend usa
    tipo              VARCHAR(20),
    categoria         nivel_alerta,
    valor             NUMERIC(14,2),
    unidade           VARCHAR(15),          -- 'mm', 'L', 'BRL'
    gerado_em         TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    detalhes          JSONB,                -- insumos + explicação
    CONSTRAINT fk_previsao_lote FOREIGN KEY (fk_lote_id_lote)
        REFERENCES lote (id_lote) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS previsao_climatica (
    fk_previsao_id_previsao         INTEGER NOT NULL,
    fk_leitura_climatica_id_leitura INTEGER NOT NULL,
    PRIMARY KEY (fk_previsao_id_previsao, fk_leitura_climatica_id_leitura),
    CONSTRAINT fk_previsao_climatica_previsao FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao) ON DELETE CASCADE,
    CONSTRAINT fk_previsao_climatica_leitura FOREIGN KEY (fk_leitura_climatica_id_leitura)
        REFERENCES leitura_climatica (id_leitura) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS previsao_mercado (
    fk_dados_mercado_id_mercado INTEGER NOT NULL,
    fk_previsao_id_previsao     INTEGER NOT NULL,
    PRIMARY KEY (fk_previsao_id_previsao, fk_dados_mercado_id_mercado),
    CONSTRAINT fk_previsao_mercado_dados FOREIGN KEY (fk_dados_mercado_id_mercado)
        REFERENCES dados_mercado (id_mercado) ON DELETE CASCADE,
    CONSTRAINT fk_previsao_mercado_previsao FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS alerta (
    id_alerta               INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    tipo_alerta             VARCHAR(150),
    descricao               VARCHAR(150),
    nivel                   nivel_alerta,
    data_hora               TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),  -- UTC
    status                  VARCHAR(150),     -- aberto | reconhecido | resolvido
    resolvido_em            TIMESTAMP,        -- UTC
    fk_previsao_id_previsao INTEGER,
    fk_leitura_id_leitura   INTEGER,
    CONSTRAINT fk_alerta_previsao FOREIGN KEY (fk_previsao_id_previsao)
        REFERENCES previsao (id_previsao) ON DELETE CASCADE,
    CONSTRAINT fk_alerta_leitura FOREIGN KEY (fk_leitura_id_leitura)
        REFERENCES leitura_climatica (id_leitura) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS log_acesso (
    id_log                INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    data_hora             TIMESTAMP,
    acao                  VARCHAR(150),
    ip                    INET,
    resultado             VARCHAR(150),
    fk_usuario_id_usuario INTEGER,   -- pode ser NULL: tentativa de login com e-mail inexistente
    CONSTRAINT fk_log_acesso_usuario FOREIGN KEY (fk_usuario_id_usuario)
        REFERENCES usuario (id_usuario) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS relatorio (
    id_relatorio          INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    tipo                  VARCHAR(150),
    descricao             VARCHAR(150),
    data_geracao          DATE,
    inicio                DATE,
    fim                   DATE,
    parametros            JSONB,   -- filtros do relatório (id_lote, agrupamento...), usado por relatorios_gerados.py
    fk_usuario_id_usuario INTEGER,
    CONSTRAINT fk_relatorio_usuario FOREIGN KEY (fk_usuario_id_usuario)
        REFERENCES usuario (id_usuario) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS porto (
    id_porto  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome      VARCHAR(80)  NOT NULL UNIQUE,
    uf        CHAR(2)      NOT NULL,
    latitude  NUMERIC(9,6) NOT NULL,
    longitude NUMERIC(9,6) NOT NULL
);

CREATE TABLE IF NOT EXISTS rota (
    id_rota           INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_porto_id_porto INTEGER NOT NULL UNIQUE,
    origem_nome       VARCHAR(80) NOT NULL DEFAULT 'Petrolina',
    distancia_km      NUMERIC(8,1),
    duracao_min       INTEGER,
    tracado           JSONB,                -- GeoJSON da API de rotas
    atualizado_em     TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_rota_porto FOREIGN KEY (fk_porto_id_porto)
        REFERENCES porto (id_porto) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS viagem (
    id_viagem         INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_lote_id_lote   INTEGER NOT NULL,
    fk_porto_id_porto INTEGER NOT NULL,
    saida             TIMESTAMP NOT NULL,
    chegada_prevista  TIMESTAMP,
    status            VARCHAR(15) NOT NULL DEFAULT 'programada',
    CONSTRAINT fk_viagem_lote FOREIGN KEY (fk_lote_id_lote)
        REFERENCES lote (id_lote) ON DELETE RESTRICT,
    CONSTRAINT fk_viagem_porto FOREIGN KEY (fk_porto_id_porto)
        REFERENCES porto (id_porto) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS leitura_transporte (
    id_leitura_transporte INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_viagem_id_viagem   INTEGER NOT NULL,
    data_hora             TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    temperatura_camara    DOUBLE PRECISION,
    umidade_camara        NUMERIC(5,2),
    latitude              NUMERIC(9,6),
    longitude             NUMERIC(9,6),
    CONSTRAINT fk_leitura_transporte_viagem FOREIGN KEY (fk_viagem_id_viagem)
        REFERENCES viagem (id_viagem) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS perda_risco (
    categoria    nivel_alerta PRIMARY KEY,
    fracao_perda NUMERIC(4,3) NOT NULL
);

/* ===== ALERTAS PREDITIVOS (usadas pelo backend: nomes, colunas, tipos e valores originais) =====
   nivel: 'BAIXO' | 'MEDIO' | 'CRITICO'   status: 'EM ABERTO' | 'RECONHECIDO' ...
   Probabilidade de 0 a 1. A faixa de cada corte é editável pela API (PUT /alertas-preditivos/cortes/{nivel}). */
CREATE TABLE IF NOT EXISTS cortes_alerta_preditivo (
    id_corte SERIAL PRIMARY KEY,
    nivel VARCHAR(20) NOT NULL UNIQUE,
    limiar_min NUMERIC(3, 2) NOT NULL,
    limiar_max NUMERIC(3, 2) NOT NULL,
    acao_recomendada TEXT NOT NULL,
    data_atualizacao TIMESTAMP DEFAULT (now() AT TIME ZONE 'UTC')
);

CREATE TABLE IF NOT EXISTS alertas_preditivos (
    id_alerta_preditivo SERIAL PRIMARY KEY,
    id_sensor INT,
    id_lote INT,
    probabilidade_risco NUMERIC(3, 2) NOT NULL,
    nivel_severidade VARCHAR(20) NOT NULL,
    mensagem TEXT NOT NULL,
    previsao_data_hora TIMESTAMP NOT NULL,
    status VARCHAR(20) DEFAULT 'EM ABERTO',
    data_geracao TIMESTAMP DEFAULT (now() AT TIME ZONE 'UTC'),
    CONSTRAINT fk_alertas_preditivos_sensor FOREIGN KEY (id_sensor)
        REFERENCES sensor (id_sensor) ON DELETE SET NULL,
    CONSTRAINT fk_alertas_preditivos_lote FOREIGN KEY (id_lote)
        REFERENCES lote (id_lote) ON DELETE RESTRICT
);

/* ===== ATUALIZAÇÃO de bancos locais antigos (no banco novo não faz nada) =====
   Só cobre colunas novas. Mudanças de FK/tipo/PK feitas nesta revisão
   (ON DELETE RESTRICT, chaves das tabelas de ligação etc.)
   exigem RECRIAR o banco (inclusive a remoção da coluna temperatura_solo, que não existe mais): não há dados a preservar antes da importação. */
ALTER TABLE sensor            ADD COLUMN IF NOT EXISTS ambiente VARCHAR(10) NOT NULL DEFAULT 'ar';
ALTER TABLE config_parametro  ADD COLUMN IF NOT EXISTS etapa    VARCHAR(15) NOT NULL DEFAULT 'armazenamento';
ALTER TABLE lote
    ADD COLUMN IF NOT EXISTS unidade       VARCHAR(10) NOT NULL DEFAULT 'kg',
    ADD COLUMN IF NOT EXISTS peso_caixa_kg NUMERIC(6,2),
    ADD COLUMN IF NOT EXISTS fk_variedade_id_variedade     INTEGER
        CONSTRAINT fk_lote_variedade   REFERENCES variedade (id_variedade) ON DELETE RESTRICT,
    ADD COLUMN IF NOT EXISTS fk_safra_id_safra             INTEGER
        CONSTRAINT fk_lote_safra       REFERENCES safra (id_safra) ON DELETE RESTRICT,
    ADD COLUMN IF NOT EXISTS fk_propriedade_id_propriedade INTEGER
        CONSTRAINT fk_lote_propriedade REFERENCES propriedade (id_propriedade) ON DELETE RESTRICT;
ALTER TABLE relatorio         ADD COLUMN IF NOT EXISTS parametros JSONB;
ALTER TABLE dados_mercado     ADD COLUMN IF NOT EXISTS unidade  VARCHAR(10) NOT NULL DEFAULT 'kg';
ALTER TABLE dados_mercado     ADD COLUMN IF NOT EXISTS moeda    CHAR(3)     NOT NULL DEFAULT 'BRL';
ALTER TABLE leitura_climatica
    ADD COLUMN IF NOT EXISTS entry_id_thingspeak INTEGER,
    ADD COLUMN IF NOT EXISTS umidade_solo        NUMERIC(5,2);
ALTER TABLE previsao
    ADD COLUMN IF NOT EXISTS tipo      VARCHAR(20),
    ADD COLUMN IF NOT EXISTS categoria nivel_alerta,
    ADD COLUMN IF NOT EXISTS valor     NUMERIC(14,2),
    ADD COLUMN IF NOT EXISTS unidade   VARCHAR(15),
    ADD COLUMN IF NOT EXISTS gerado_em TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),
    ADD COLUMN IF NOT EXISTS detalhes  JSONB;
ALTER TABLE alerta ADD COLUMN IF NOT EXISTS resolvido_em TIMESTAMP;

DO $$
BEGIN
    -- alerta.hora (TIME) -> data_hora (TIMESTAMP)
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_name = 'alerta' AND column_name = 'hora') THEN
        ALTER TABLE alerta ADD COLUMN IF NOT EXISTS data_hora TIMESTAMP;
        EXECUTE 'UPDATE alerta SET data_hora = CURRENT_DATE + hora WHERE data_hora IS NULL';
        EXECUTE 'UPDATE alerta SET data_hora = (now() AT TIME ZONE ''UTC'') WHERE data_hora IS NULL';
        ALTER TABLE alerta ALTER COLUMN data_hora SET DEFAULT (now() AT TIME ZONE 'UTC');
        ALTER TABLE alerta ALTER COLUMN data_hora SET NOT NULL;
        ALTER TABLE alerta DROP COLUMN hora;
    END IF;

    -- log_acesso.ip INTEGER -> INET
    IF (SELECT data_type FROM information_schema.columns
        WHERE table_name = 'log_acesso' AND column_name = 'ip') = 'integer' THEN
        ALTER TABLE log_acesso ALTER COLUMN ip TYPE INET USING NULL;
    END IF;
END $$;

/* ===== ÍNDICES (depois da atualização, pois dependem de colunas novas) ===== */
-- leituras
CREATE INDEX IF NOT EXISTS idx_leitura_sensor_data ON leitura_climatica (fk_sensor_id_sensor, data_hora);
CREATE INDEX IF NOT EXISTS idx_rejeitada_sensor_data ON leitura_rejeitada (fk_sensor_id_sensor, data_rejeicao);
CREATE INDEX IF NOT EXISTS idx_leitura_transporte_viagem ON leitura_transporte (fk_viagem_id_viagem, data_hora);
-- lote e vínculos
CREATE INDEX IF NOT EXISTS idx_lote_fruta       ON lote (fk_fruta_id_fruta);
CREATE INDEX IF NOT EXISTS idx_lote_variedade   ON lote (fk_variedade_id_variedade);
CREATE INDEX IF NOT EXISTS idx_lote_safra       ON lote (fk_safra_id_safra);
CREATE INDEX IF NOT EXISTS idx_lote_propriedade ON lote (fk_propriedade_id_propriedade);
CREATE INDEX IF NOT EXISTS idx_lote_status      ON lote (status);
CREATE INDEX IF NOT EXISTS idx_sensor_lote      ON sensor (fk_lote_id_lote);
-- previsão, alerta, mercado
CREATE INDEX IF NOT EXISTS idx_previsao_lote_tipo
    ON previsao (fk_lote_id_lote, tipo, gerado_em DESC);
CREATE INDEX IF NOT EXISTS idx_previsao_climatica_leitura ON previsao_climatica (fk_leitura_climatica_id_leitura);
CREATE INDEX IF NOT EXISTS idx_previsao_mercado_dados     ON previsao_mercado (fk_dados_mercado_id_mercado);
CREATE INDEX IF NOT EXISTS idx_alerta_data_hora     ON alerta (data_hora);
CREATE INDEX IF NOT EXISTS idx_alerta_leitura       ON alerta (fk_leitura_id_leitura);
CREATE INDEX IF NOT EXISTS idx_alerta_previsao      ON alerta (fk_previsao_id_previsao);
CREATE INDEX IF NOT EXISTS idx_alerta_status_data   ON alerta (status, data_hora DESC);
CREATE INDEX IF NOT EXISTS idx_dados_mercado_fruta  ON dados_mercado (fk_fruta_id_fruta, data_refencia);
-- logística
CREATE INDEX IF NOT EXISTS idx_viagem_status ON viagem (status);
CREATE INDEX IF NOT EXISTS idx_viagem_lote   ON viagem (fk_lote_id_lote);
CREATE INDEX IF NOT EXISTS idx_viagem_porto  ON viagem (fk_porto_id_porto);
-- alertas preditivos
CREATE INDEX IF NOT EXISTS idx_alertas_preditivos_lote   ON alertas_preditivos (id_lote, status, data_geracao DESC);
CREATE INDEX IF NOT EXISTS idx_alertas_preditivos_sensor ON alertas_preditivos (id_sensor);
-- acesso
CREATE INDEX IF NOT EXISTS idx_log_acesso_data ON log_acesso (data_hora);
CREATE INDEX IF NOT EXISTS idx_usuario_perfil  ON usuario (fk_perfil_id_perfil);
CREATE INDEX IF NOT EXISTS idx_relatorio_usuario ON relatorio (fk_usuario_id_usuario);
