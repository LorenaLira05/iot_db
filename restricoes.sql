-- restricoes.sql
-- Restrições de integridade. Idempotente. Roda depois do schema.sql.
-- Se falhar por dados que violam a regra, corrija os dados e rode de novo
-- (a transação desfaz tudo; sem "BEGIN" aberto na sessão, não precisa de ROLLBACK manual).

BEGIN;

-- usuario: e-mail único (sem diferenciar maiúsculas) e campos obrigatórios
CREATE UNIQUE INDEX IF NOT EXISTS uq_usuario_email_lower ON usuario (lower(email));
ALTER TABLE usuario ALTER COLUMN nome                SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN email               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN senha               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN status              SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN fk_perfil_id_perfil SET NOT NULL;

-- chaves naturais únicas (seed e trigger dependem disso)
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_perfil_nome') THEN
        ALTER TABLE perfil ADD CONSTRAINT uq_perfil_nome UNIQUE (nome);
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_fruta_nome') THEN
        ALTER TABLE fruta ADD CONSTRAINT uq_fruta_nome UNIQUE (nome);
    END IF;
    -- uma faixa por fruta E ETAPA (a antiga uq_config_fruta impedia 2 etapas)
    IF EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_config_fruta') THEN
        ALTER TABLE config_parametro DROP CONSTRAINT uq_config_fruta;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_config_fruta_etapa') THEN
        ALTER TABLE config_parametro
            ADD CONSTRAINT uq_config_fruta_etapa UNIQUE (fk_fruta_id_fruta, etapa);
    END IF;
    -- channel_id NULL pode repetir (sensores simulados); preenchido é único
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_sensor_channel') THEN
        ALTER TABLE sensor ADD CONSTRAINT uq_sensor_channel UNIQUE (channel_id);
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_leitura_sensor_entry') THEN
        ALTER TABLE leitura_climatica
            ADD CONSTRAINT uq_leitura_sensor_entry UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak);
    END IF;
END $$;

-- CHECKs
ALTER TABLE sensor DROP CONSTRAINT IF EXISTS ck_sensor_ambiente;
ALTER TABLE sensor ADD CONSTRAINT ck_sensor_ambiente CHECK (ambiente IN ('ar','solo'));

ALTER TABLE config_parametro DROP CONSTRAINT IF EXISTS ck_config_etapa;
ALTER TABLE config_parametro ADD CONSTRAINT ck_config_etapa
    CHECK (etapa IN ('campo','armazenamento','transporte'));

ALTER TABLE previsao DROP CONSTRAINT IF EXISTS ck_previsao_tipo;
ALTER TABLE previsao ADD CONSTRAINT ck_previsao_tipo
    CHECK (tipo IS NULL OR tipo IN ('risco_climatico','irrigacao','prejuizo','janela_colheita'));

ALTER TABLE lote DROP CONSTRAINT IF EXISTS ck_lote_unidade;
ALTER TABLE lote ADD CONSTRAINT ck_lote_unidade CHECK (unidade IN ('kg','caixa','t'));

ALTER TABLE viagem DROP CONSTRAINT IF EXISTS ck_viagem_status;
ALTER TABLE viagem ADD CONSTRAINT ck_viagem_status
    CHECK (status IN ('programada','em_transporte','concluida','cancelada'));

ALTER TABLE perda_risco DROP CONSTRAINT IF EXISTS ck_perda_fracao;
ALTER TABLE perda_risco ADD CONSTRAINT ck_perda_fracao CHECK (fracao_perda BETWEEN 0 AND 1);

COMMIT;
-- fk_sensor_id_sensor NOT NULL em leitura_climatica é aplicado no fim do seed.sql.
