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
    fk_sensor_id_sensor INTEGER,

    CONSTRAINT fk_leitura_climatica_sensor
        FOREIGN KEY (fk_sensor_id_sensor)
        REFERENCES sensor (id_sensor)
        ON DELETE RESTRICT
);
CREATE INDEX idx_leitura_sensor_data ON leitura_climatica (fk_sensor_id_sensor, data_hora);


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
