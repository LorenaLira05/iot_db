/* IA MODELO */

CREATE TABLE IF NOT EXISTS ia_modelo ( 
    id_modelo INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 
    nome VARCHAR(150) NOT NULL, 
    tipo_modelo VARCHAR(100) NOT NULL, 
    finalidade VARCHAR(150) NOT NULL, 
    algoritmo VARCHAR(150), 
    versao VARCHAR(50), 
    status VARCHAR(50) NOT NULL, 
    data_criacao TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    data_atualizacao TIMESTAMP 
); 


/* CHAT CONVERSA */


CREATE TABLE IF NOT EXISTS chat_conversa ( 
    id_conversa INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 
    fk_usuario_id_usuario INTEGER NOT NULL, 
    titulo VARCHAR(150), 
    data_inicio TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    data_ultima_mensagem TIMESTAMP, 

    CONSTRAINT fk_chat_conversa_usuario 
        FOREIGN KEY (fk_usuario_id_usuario) 
        REFERENCES usuario (id_usuario) 
        ON DELETE CASCADE 
); 

/* CHAT MENSAGEM */

CREATE TABLE IF NOT EXISTS chat_mensagem ( 
    id_mensagem INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 
    fk_conversa_id_conversa INTEGER NOT NULL, 
    tipo VARCHAR(20) NOT NULL, 
    mensagem TEXT NOT NULL, 
    data_hora TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    modelo_utilizado VARCHAR(150), 

    CONSTRAINT fk_chat_mensagem_conversa 
        FOREIGN KEY (fk_conversa_id_conversa) 
        REFERENCES chat_conversa (id_conversa) 
        ON DELETE CASCADE 
); 

/* PREVISAO PREJUIZO */

CREATE TABLE IF NOT EXISTS previsao_prejuizo ( 
    id_previsao_prejuizo INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 

    fk_lote_id_lote INTEGER NOT NULL, 
    fk_modelo_id_modelo INTEGER NOT NULL, 

    temperatura DOUBLE PRECISION, 
    umidade NUMERIC(5,2), 

    periodo_inicio DATE, 
    periodo_fim DATE, 

    prejuizo_estimado NUMERIC(12,2), 

    data_previsao TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 

    CONSTRAINT fk_prejuizo_lote 
        FOREIGN KEY (fk_lote_id_lote) 
        REFERENCES lote (id_lote) 
        ON DELETE CASCADE, 

    CONSTRAINT fk_prejuizo_modelo 
        FOREIGN KEY (fk_modelo_id_modelo) 
        REFERENCES ia_modelo (id_modelo) 
        ON DELETE RESTRICT 
); 

/* ALERTA PREDITIVO */

CREATE TABLE IF NOT EXISTS alerta_preditivo ( 
    id_alerta_preditivo INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 

    fk_lote_id_lote INTEGER NOT NULL, 
    fk_modelo_id_modelo INTEGER NOT NULL, 

    temperatura DOUBLE PRECISION, 
    umidade NUMERIC(5,2), 

    classificacao VARCHAR(50) NOT NULL, 
    probabilidade NUMERIC(5,2), 

    motivo TEXT, 

    data_previsao TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    status VARCHAR(50) DEFAULT 'aberto', 

    CONSTRAINT fk_alerta_preditivo_lote 
        FOREIGN KEY (fk_lote_id_lote) 
        REFERENCES lote (id_lote) 
        ON DELETE CASCADE, 

    CONSTRAINT fk_alerta_preditivo_modelo 
        FOREIGN KEY (fk_modelo_id_modelo) 
        REFERENCES ia_modelo (id_modelo) 
        ON DELETE RESTRICT 
); 

/* PREVISAO IRRIGAÇÃO */

CREATE TABLE IF NOT EXISTS previsao_irrigacao ( 
    id_previsao_irrigacao INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 

    fk_lote_id_lote INTEGER NOT NULL, 
    fk_modelo_classificacao INTEGER NOT NULL, 
    fk_modelo_regressao INTEGER, 

  
    temperatura DOUBLE PRECISION, 
    umidade NUMERIC(5,2), 
 

    necessita_irrigacao BOOLEAN NOT NULL, 
    quantidade_agua_mm NUMERIC(8,2), 
    data_previsao TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    CONSTRAINT fk_irrigacao_lote 
        FOREIGN KEY (fk_lote_id_lote) 
        REFERENCES lote (id_lote) 
        ON DELETE CASCADE, 
    CONSTRAINT fk_irrigacao_modelo_classificacao 
        FOREIGN KEY (fk_modelo_classificacao) 
        REFERENCES ia_modelo (id_modelo) 
        ON DELETE RESTRICT, 
    CONSTRAINT fk_irrigacao_modelo_regressao 
        FOREIGN KEY (fk_modelo_regressao) 
        REFERENCES ia_modelo (id_modelo) 
        ON DELETE RESTRICT 
); 

/* RISCO CLIMATICO */

CREATE TABLE IF NOT EXISTS risco_climatico ( 
    id_risco_climatico INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY, 
    fk_lote_id_lote INTEGER NOT NULL, 
    fk_modelo_id_modelo INTEGER NOT NULL, 
    temperatura DOUBLE PRECISION, 
    umidade NUMERIC(5,2), 
    periodo_inicio DATE, 
    periodo_fim DATE, 
    nivel_risco VARCHAR(50) NOT NULL, 
    probabilidade NUMERIC(5,2), 

    justificativa TEXT, 
    data_previsao TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 

    CONSTRAINT fk_risco_lote 
        FOREIGN KEY (fk_lote_id_lote) 
        REFERENCES lote (id_lote) 
        ON DELETE CASCADE, 

    CONSTRAINT fk_risco_modelo 
        FOREIGN KEY (fk_modelo_id_modelo) 
        REFERENCES ia_modelo (id_modelo) 
        ON DELETE RESTRICT 
);

/* PARAMETRO_PREJUIZO*/

CREATE TABLE parametro_prejuizo (
    id_parametro INTEGER
        GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 
    nivel_risco VARCHAR(20) NOT NULL UNIQUE,
 
    fracao_perda NUMERIC(5,4) NOT NULL,
    tolerancia_horas NUMERIC(5,2) NOT NULL,
    acrescimo_hora NUMERIC(5,4) NOT NULL,
 
    periodo_analise_dias INTEGER NOT NULL,
    validade_preco_dias INTEGER NOT NULL,
 
    justificativa TEXT NOT NULL,
    aprovado BOOLEAN NOT NULL DEFAULT FALSE,
    data_aprovacao TIMESTAMP,
    responsavel_aprovacao VARCHAR(150),
 
    CONSTRAINT chk_parametro_risco
        CHECK (nivel_risco IN ('baixo', 'médio', 'alto')),
 
    CONSTRAINT chk_fracao_perda
        CHECK (fracao_perda BETWEEN 0 AND 1)
);

/* PARAMETRO_IRRIGACAO */

CREATE TABLE parametro_irrigacao (
    id_parametro INTEGER
        GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 
    fk_fruta_id_fruta INTEGER NOT NULL UNIQUE
        REFERENCES fruta(id_fruta),
 
    kc NUMERIC(5,3) NOT NULL,
    fase_cultura VARCHAR(80) NOT NULL,
 
    chuva_efetiva_percentual NUMERIC(5,2) NOT NULL,
    probabilidade_chuva_minima NUMERIC(5,2) NOT NULL,
 
    irrigacao_minima_mm NUMERIC(6,2) NOT NULL,
    vpd_limite_kpa NUMERIC(5,2) NOT NULL,
 
    umidade_solo_minima NUMERIC(5,2) NOT NULL,
 
    justificativa TEXT NOT NULL,
    aprovado BOOLEAN NOT NULL DEFAULT FALSE,
 
    data_aprovacao TIMESTAMP,
    responsavel_aprovacao VARCHAR(150),
 
    CONSTRAINT chk_kc
        CHECK (kc > 0),
 
    CONSTRAINT chk_chuva_efetiva
        CHECK (chuva_efetiva_percentual BETWEEN 0 AND 100),
 
    CONSTRAINT chk_probabilidade_chuva
        CHECK (probabilidade_chuva_minima BETWEEN 0 AND 100),
 
    CONSTRAINT chk_irrigacao_minima
        CHECK (irrigacao_minima_mm >= 0),
 
    CONSTRAINT chk_vpd
        CHECK (vpd_limite_kpa > 0),
 
    CONSTRAINT chk_umidade_solo
        CHECK (umidade_solo_minima BETWEEN 0 AND 100)
);

-- parecer da IA e gerir o cache por lote
CREATE TABLE IF NOT EXISTS parecer_ia (
    id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    id_lote INTEGER NOT NULL REFERENCES lote(id) ON DELETE CASCADE,
    modelo VARCHAR(100) NOT NULL,
    snapshot_entrada JSONB NOT NULL,
    conteudo_json JSONB NOT NULL,
    data_geracao TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    data_expiracao TIMESTAMP WITH TIME ZONE NOT NULL
);

-- Índice para acelerar a busca de cache ativo por lote
CREATE INDEX IF NOT EXISTS idx_parecer_ia_lote_expiracao 
ON parecer_ia (id_lote, data_expiracao DESC);
