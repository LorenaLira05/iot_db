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

