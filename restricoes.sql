-- restricoes.sql
-- Restrições que faltavam no schema. Pode rodar mais de uma vez.
-- Ordem sugerida: schema -> restricoes.sql -> seed.sql
-- Se algum passo falhar por dados existentes que violam a regra, corrija os
-- dados e rode de novo (a transação desfaz tudo).

BEGIN;

-- leitura_climatica: deduplicação da ingestão do ThingSpeak
ALTER TABLE leitura_climatica
    ADD COLUMN IF NOT EXISTS entry_id_thingspeak INTEGER;

DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'uq_leitura_sensor_entry'
    ) THEN
        ALTER TABLE leitura_climatica
            ADD CONSTRAINT uq_leitura_sensor_entry
            UNIQUE (fk_sensor_id_sensor, entry_id_thingspeak);
    END IF;
END $$;

-- usuario: e-mail único (sem diferenciar maiúsculas) e campos obrigatórios
CREATE UNIQUE INDEX IF NOT EXISTS uq_usuario_email_lower
    ON usuario (lower(email));

ALTER TABLE usuario ALTER COLUMN nome                SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN email               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN senha               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN status              SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN fk_perfil_id_perfil SET NOT NULL;

-- perfil, fruta, sensor: chaves naturais únicas
-- (o seed.sql depende disso para não duplicar)
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_perfil_nome') THEN
        ALTER TABLE perfil ADD CONSTRAINT uq_perfil_nome UNIQUE (nome);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_fruta_nome') THEN
        ALTER TABLE fruta ADD CONSTRAINT uq_fruta_nome UNIQUE (nome);
    END IF;

    -- uma faixa ideal por fruta (o trigger de alerta e o seed dependem disso)
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_config_fruta') THEN
        ALTER TABLE config_parametro ADD CONSTRAINT uq_config_fruta UNIQUE (fk_fruta_id_fruta);
    END IF;

    -- channel_id NULL pode repetir (sensores simulados); só o valor preenchido é único
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_sensor_channel') THEN
        ALTER TABLE sensor ADD CONSTRAINT uq_sensor_channel UNIQUE (channel_id);
    END IF;
END $$;

COMMIT;
