-- =====================================================================
-- Migração de AUTENTICAÇÃO (login, refresh token, 2º fator por e-mail)
-- Pode ser rodada mais de uma vez (idempotente). Não altera nenhuma
-- tabela existente de dados: só cria duas tabelas, um índice e, se
-- preciso, alarga uma coluna de log_acesso.
-- =====================================================================

-- 1) Sessões: refresh tokens com rotação -------------------------------
-- O token em si NUNCA é salvo, só o hash (sha256). Cada refresh usado é
-- marcado em usado_em e trocado por um novo. Se alguém reapresentar um
-- token já usado (possível roubo), toda a "familia" (sessão) é revogada.
CREATE TABLE IF NOT EXISTS refresh_token (
    id_refresh_token       BIGSERIAL PRIMARY KEY,
    fk_usuario_id_usuario  INTEGER     NOT NULL REFERENCES usuario(id_usuario) ON DELETE CASCADE,
    familia                UUID        NOT NULL,
    token_hash             CHAR(64)    NOT NULL UNIQUE,
    criado_em              TIMESTAMPTZ NOT NULL DEFAULT now(),
    expira_em              TIMESTAMPTZ NOT NULL,
    usado_em               TIMESTAMPTZ,
    revogado_em            TIMESTAMPTZ,
    ip                     TEXT,
    user_agent             TEXT
);
CREATE INDEX IF NOT EXISTS idx_refresh_token_usuario ON refresh_token (fk_usuario_id_usuario);
CREATE INDEX IF NOT EXISTS idx_refresh_token_familia ON refresh_token (familia);

-- 2) Segundo fator: código de 6 dígitos enviado por e-mail -------------
-- O código NUNCA é salvo, só o HMAC dele (chave EMAIL_OTP_KEY do .env).
-- Vale 10 min, uso único, 5 erros por código; um código novo invalida o anterior.
CREATE TABLE IF NOT EXISTS codigo_2fa_email (
    id_codigo              BIGSERIAL PRIMARY KEY,
    fk_usuario_id_usuario  INTEGER     NOT NULL REFERENCES usuario(id_usuario) ON DELETE CASCADE,
    codigo_hash            CHAR(64)    NOT NULL,
    criado_em              TIMESTAMPTZ NOT NULL DEFAULT now(),
    expira_em              TIMESTAMPTZ NOT NULL,
    tentativas             SMALLINT    NOT NULL DEFAULT 0,
    usado_em               TIMESTAMPTZ
);
CREATE INDEX IF NOT EXISTS idx_codigo_2fa_email_usuario
    ON codigo_2fa_email (fk_usuario_id_usuario, criado_em DESC);

-- 3) Auditoria: índice para o bloqueio por tentativas ------------------
-- A cada login contamos as falhas recentes do usuário em log_acesso;
-- o índice mantém essa consulta rápida (RNF09: RBAC sem degradar a performance).
CREATE INDEX IF NOT EXISTS idx_log_acesso_usuario_acao
    ON log_acesso (fk_usuario_id_usuario, acao, data_hora DESC);

-- 4) Auditoria: alarga log_acesso.acao só se estiver pequena -----------
-- Nomes como 'login_senha_ok_codigo_enviado' passam de 20 caracteres.
DO $$
DECLARE tam integer;
BEGIN
    SELECT character_maximum_length INTO tam
      FROM information_schema.columns
     WHERE table_name = 'log_acesso' AND column_name = 'acao';
    IF tam IS NOT NULL AND tam < 60 THEN
        ALTER TABLE log_acesso ALTER COLUMN acao TYPE varchar(60);
    END IF;
END $$;

-- O primeiro Administrador NÃO entra aqui (tem hash de senha). Crie assim:
--   INSERT INTO usuario (nome, email, senha, status, fk_perfil_id_perfil)
--   VALUES ('Nome', 'email@real.com', '<hash argon2>', 'ativo',
--           (SELECT id_perfil FROM perfil WHERE nome = 'Administrador'));
