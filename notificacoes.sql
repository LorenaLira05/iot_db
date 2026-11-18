CREATE TABLE IF NOT EXISTS notificacao_preferencia (
    fk_usuario_id_usuario INTEGER NOT NULL,
    canal                 VARCHAR(20) NOT NULL,
    ativo                 BOOLEAN NOT NULL DEFAULT TRUE,
    nivel_minimo          nivel_alerta NOT NULL DEFAULT 'baixo',
    data_atualizacao      TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),

    PRIMARY KEY (fk_usuario_id_usuario, canal),
    CONSTRAINT fk_notif_pref_usuario
        FOREIGN KEY (fk_usuario_id_usuario) REFERENCES usuario (id_usuario) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS notificacao_envio (
    id_notificacao_envio  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fk_usuario_id_usuario INTEGER NOT NULL,
    fk_alerta_id_alerta   INTEGER NOT NULL,
    canal                 VARCHAR(20) NOT NULL,
    status                VARCHAR(15) NOT NULL,
    motivo                VARCHAR(200),
    chave_dedup           VARCHAR(200) NOT NULL,
    data_hora             TIMESTAMP NOT NULL DEFAULT (now() AT TIME ZONE 'UTC'),

    CONSTRAINT fk_notif_envio_usuario
        FOREIGN KEY (fk_usuario_id_usuario) REFERENCES usuario (id_usuario) ON DELETE CASCADE,
    CONSTRAINT fk_notif_envio_alerta
        FOREIGN KEY (fk_alerta_id_alerta) REFERENCES alerta (id_alerta) ON DELETE CASCADE,
    CONSTRAINT chk_notif_envio_status
        CHECK (status IN ('enviando', 'enviado', 'falha', 'suprimido', 'ignorado'))
);

CREATE UNIQUE INDEX IF NOT EXISTS ux_notif_envio_alerta_usuario_canal
    ON notificacao_envio (fk_alerta_id_alerta, fk_usuario_id_usuario, canal)
    WHERE status <> 'falha';

CREATE INDEX IF NOT EXISTS ix_notif_envio_dedup
    ON notificacao_envio (fk_usuario_id_usuario, canal, chave_dedup, data_hora DESC)
    WHERE status IN ('enviando', 'enviado');

CREATE INDEX IF NOT EXISTS ix_notif_envio_usuario_data
    ON notificacao_envio (fk_usuario_id_usuario, data_hora DESC);