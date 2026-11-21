/* ATIVAÇÃO DE CONTA / PRIMEIRO ACESSO */

ALTER TYPE status_usuario ADD VALUE IF NOT EXISTS 'pendente';

ALTER TABLE usuario
    ADD COLUMN IF NOT EXISTS deve_trocar_senha BOOLEAN NOT NULL DEFAULT FALSE,
    ADD COLUMN IF NOT EXISTS ativado_em        TIMESTAMP,
    ADD COLUMN IF NOT EXISTS termos_aceitos_em TIMESTAMP,
    ADD COLUMN IF NOT EXISTS termos_versao     VARCHAR(20);

-- quem já existe conta como ativado
UPDATE usuario
   SET ativado_em = COALESCE(data_cadastro::timestamp, now())
 WHERE ativado_em IS NULL;

-- Código de ativação enviado por e-mail (só o hash é guardado).
CREATE TABLE IF NOT EXISTS ativacao_token (
    id_ativacao           INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_usuario_id_usuario INTEGER NOT NULL,
    codigo_hash           CHAR(64) NOT NULL,
    tentativas            SMALLINT NOT NULL DEFAULT 0,
    criado_em             TIMESTAMP NOT NULL DEFAULT now(),
    expira_em             TIMESTAMP NOT NULL,
    usado_em              TIMESTAMP,
    ip                    VARCHAR(45),

    CONSTRAINT fk_ativacao_usuario
        FOREIGN KEY (fk_usuario_id_usuario) REFERENCES usuario (id_usuario) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS ix_ativacao_usuario
    ON ativacao_token (fk_usuario_id_usuario, criado_em DESC);
