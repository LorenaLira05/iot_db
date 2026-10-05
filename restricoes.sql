-- restricoes.sql
-- Restrições que faltavam no schema. Pode rodar mais de uma vez.
-- Ordem: schema -> restricoes.sql -> triggers -> views -> seed.sql
-- Se algum passo falhar por dados que violam a regra, a transação desfaz tudo:
-- corrija os dados, rode ROLLBACK; e execute de novo.

BEGIN;

-- ============================================================
-- leitura_climatica: deduplicação da ingestão do ThingSpeak
-- ============================================================
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

-- ============================================================
-- usuario: e-mail único (sem diferenciar maiúsculas) e campos obrigatórios
-- ============================================================
CREATE UNIQUE INDEX IF NOT EXISTS uq_usuario_email_lower
    ON usuario (lower(email));

ALTER TABLE usuario ALTER COLUMN nome                SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN email               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN senha               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN status              SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN fk_perfil_id_perfil SET NOT NULL;

-- ============================================================
-- perfil, fruta, config_parametro, sensor: chaves naturais únicas
-- (o seed.sql e o trigger de alerta dependem disso)
-- ============================================================
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_perfil_nome') THEN
        ALTER TABLE perfil ADD CONSTRAINT uq_perfil_nome UNIQUE (nome);
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_fruta_nome') THEN
        ALTER TABLE fruta ADD CONSTRAINT uq_fruta_nome UNIQUE (nome);
    END IF;

    -- uma faixa ideal por fruta E ETAPA (a antiga uq_config_fruta impedia 2 etapas)
    IF EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_config_fruta') THEN
        ALTER TABLE config_parametro DROP CONSTRAINT uq_config_fruta;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_config_fruta_etapa') THEN
        ALTER TABLE config_parametro
            ADD CONSTRAINT uq_config_fruta_etapa UNIQUE (fk_fruta_id_fruta, etapa);
    END IF;

    -- channel_id NULL pode repetir (sensores simulados); só o valor preenchido é único
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_sensor_channel') THEN
        ALTER TABLE sensor ADD CONSTRAINT uq_sensor_channel UNIQUE (channel_id);
    END IF;
END $$;

-- ============================================================
-- CHECKs das colunas novas
-- ============================================================
ALTER TABLE sensor DROP CONSTRAINT IF EXISTS ck_sensor_ambiente;
ALTER TABLE sensor ADD CONSTRAINT ck_sensor_ambiente
    CHECK (ambiente IN ('ar','solo'));

ALTER TABLE config_parametro DROP CONSTRAINT IF EXISTS ck_config_etapa;
ALTER TABLE config_parametro ADD CONSTRAINT ck_config_etapa
    CHECK (etapa IN ('campo','armazenamento','transporte'));

ALTER TABLE previsao DROP CONSTRAINT IF EXISTS ck_previsao_tipo;
ALTER TABLE previsao ADD CONSTRAINT ck_previsao_tipo
    CHECK (tipo IS NULL OR tipo IN ('risco_climatico','irrigacao','prejuizo','janela_colheita'));

ALTER TABLE lote DROP CONSTRAINT IF EXISTS ck_lote_unidade;
ALTER TABLE lote ADD CONSTRAINT ck_lote_unidade
    CHECK (unidade IN ('kg','caixa','t'));

ALTER TABLE viagem DROP CONSTRAINT IF EXISTS ck_viagem_status;
ALTER TABLE viagem ADD CONSTRAINT ck_viagem_status
    CHECK (status IN ('programada','em_transporte','concluida','cancelada'));

ALTER TABLE perda_risco DROP CONSTRAINT IF EXISTS ck_perda_fracao;
ALTER TABLE perda_risco ADD CONSTRAINT ck_perda_fracao
    CHECK (fracao_perda BETWEEN 0 AND 1);

-- ============================================================
-- Opcional (descomente quando fizer sentido)
-- ============================================================

-- log_acesso.ip é INTEGER e não guarda IPs como 192.168.0.1. Antes de usar o log:
-- ALTER TABLE log_acesso ALTER COLUMN ip TYPE INET USING NULL;
-- (o schema.sql já converte para INET)

COMMIT;

-- Obs.: fk_sensor_id_sensor NOT NULL em leitura_climatica é aplicado no fim do
-- seed.sql, depois que o sensor existe e as leituras antigas foram ligadas a ele.
