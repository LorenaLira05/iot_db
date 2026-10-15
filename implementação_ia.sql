/* IA MODELO */

CREATE TABLE ia_modelo ( 
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


CREATE TABLE chat_conversa ( 
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

CREATE TABLE chat_mensagem ( 
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

CREATE TABLE previsao_prejuizo ( 
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

CREATE TABLE alerta_preditivo ( 
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

CREATE TABLE previsao_irrigacao ( 
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

CREATE TABLE risco_climatico ( 
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
