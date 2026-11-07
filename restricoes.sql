-- restricoes.sql
-- Restrições de integridade. Idempotente. Roda depois do schema.sql e do implementação_ia.sql.
-- Se falhar por dados que violam a regra, corrija os dados e rode de novo
-- (a transação desfaz tudo; sem "BEGIN" aberto na sessão, não precisa de ROLLBACK manual).
-- Importe os dados DEPOIS deste script: assim cada linha já é validada na entrada.

BEGIN;

/* ===== NOT NULL e DEFAULT ===== */

-- usuario: e-mail único (sem diferenciar maiúsculas) e campos obrigatórios
CREATE UNIQUE INDEX IF NOT EXISTS uq_usuario_email_lower ON usuario (lower(email));
ALTER TABLE usuario ALTER COLUMN nome                SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN email               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN senha               SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN status              SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN fk_perfil_id_perfil SET NOT NULL;
ALTER TABLE usuario ALTER COLUMN status              SET DEFAULT 'ativo';
ALTER TABLE usuario ALTER COLUMN data_cadastro       SET DEFAULT ((now() AT TIME ZONE 'UTC')::date);

ALTER TABLE perfil ALTER COLUMN nome SET NOT NULL;
ALTER TABLE fruta  ALTER COLUMN nome SET NOT NULL;

ALTER TABLE lote ALTER COLUMN fk_fruta_id_fruta SET NOT NULL;

ALTER TABLE sensor ALTER COLUMN nome   SET NOT NULL;
ALTER TABLE sensor ALTER COLUMN status SET NOT NULL;
ALTER TABLE sensor ALTER COLUMN status SET DEFAULT 'ativo';

-- toda leitura pertence a um sensor e tem horário de medição (antes isso ficava no fim do seed.sql)
ALTER TABLE leitura_climatica ALTER COLUMN fk_sensor_id_sensor SET NOT NULL;
ALTER TABLE leitura_climatica ALTER COLUMN data_hora           SET NOT NULL;
ALTER TABLE leitura_climatica ALTER COLUMN data_recebimento    SET DEFAULT (now() AT TIME ZONE 'UTC');

ALTER TABLE config_parametro ALTER COLUMN fk_fruta_id_fruta SET NOT NULL;
ALTER TABLE config_parametro ALTER COLUMN temp_min          SET NOT NULL;
ALTER TABLE config_parametro ALTER COLUMN temp_max          SET NOT NULL;
ALTER TABLE config_parametro ALTER COLUMN umidade_min       SET NOT NULL;
ALTER TABLE config_parametro ALTER COLUMN umidade_max       SET NOT NULL;

ALTER TABLE dados_mercado ALTER COLUMN fk_fruta_id_fruta SET NOT NULL;
ALTER TABLE previsao      ALTER COLUMN fk_lote_id_lote   SET NOT NULL;

ALTER TABLE alerta ALTER COLUMN tipo_alerta SET NOT NULL;
ALTER TABLE alerta ALTER COLUMN nivel       SET NOT NULL;
ALTER TABLE alerta ALTER COLUMN status      SET NOT NULL;
ALTER TABLE alerta ALTER COLUMN status      SET DEFAULT 'aberto';

ALTER TABLE log_acesso ALTER COLUMN data_hora SET NOT NULL;
ALTER TABLE log_acesso ALTER COLUMN data_hora SET DEFAULT (now() AT TIME ZONE 'UTC');
ALTER TABLE log_acesso ALTER COLUMN acao      SET NOT NULL;

/* ===== UNIQUE (chaves naturais; seed e trigger dependem disso) ===== */
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

/* ===== CHECKs: vocabulários fechados ===== */
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

ALTER TABLE lote_classificacao DROP CONSTRAINT IF EXISTS ck_classificacao_categoria;
ALTER TABLE lote_classificacao ADD CONSTRAINT ck_classificacao_categoria
    CHECK (categoria IN ('exportacao','mercado_interno','processamento'));

ALTER TABLE alerta DROP CONSTRAINT IF EXISTS ck_alerta_status;
ALTER TABLE alerta ADD CONSTRAINT ck_alerta_status
    CHECK (status IN ('aberto','reconhecido','resolvido'));

/* ===== CHECKs: faixas de valores e coerência ===== */
ALTER TABLE perda_risco DROP CONSTRAINT IF EXISTS ck_perda_fracao;
ALTER TABLE perda_risco ADD CONSTRAINT ck_perda_fracao CHECK (fracao_perda BETWEEN 0 AND 1);

ALTER TABLE config_parametro DROP CONSTRAINT IF EXISTS ck_config_temp;
ALTER TABLE config_parametro ADD CONSTRAINT ck_config_temp CHECK (temp_min < temp_max);
ALTER TABLE config_parametro DROP CONSTRAINT IF EXISTS ck_config_umidade;
ALTER TABLE config_parametro ADD CONSTRAINT ck_config_umidade
    CHECK (umidade_min >= 0 AND umidade_max <= 100 AND umidade_min < umidade_max);

-- leitura: umidade em %, temperatura em faixa física plausível.
-- Valor fora disso é erro de sensor e deve ir para leitura_rejeitada, não para leitura_climatica.
ALTER TABLE leitura_climatica DROP CONSTRAINT IF EXISTS ck_leitura_valores;
ALTER TABLE leitura_climatica ADD CONSTRAINT ck_leitura_valores CHECK (
        (umidade          IS NULL OR umidade          BETWEEN 0 AND 100)
    AND (umidade_solo     IS NULL OR umidade_solo     BETWEEN 0 AND 100)
    AND (temperatura      IS NULL OR temperatura      BETWEEN -50 AND 100)
    AND (temperatura_solo IS NULL OR temperatura_solo BETWEEN -50 AND 100));
ALTER TABLE leitura_climatica DROP CONSTRAINT IF EXISTS ck_leitura_alguma_medida;
ALTER TABLE leitura_climatica ADD CONSTRAINT ck_leitura_alguma_medida CHECK (
    temperatura IS NOT NULL OR umidade IS NOT NULL
    OR temperatura_solo IS NOT NULL OR umidade_solo IS NOT NULL);

ALTER TABLE leitura_transporte DROP CONSTRAINT IF EXISTS ck_transporte_umidade;
ALTER TABLE leitura_transporte ADD CONSTRAINT ck_transporte_umidade
    CHECK (umidade_camara IS NULL OR umidade_camara BETWEEN 0 AND 100);

ALTER TABLE lote DROP CONSTRAINT IF EXISTS ck_lote_quantidade;
ALTER TABLE lote ADD CONSTRAINT ck_lote_quantidade CHECK (quantidade >= 0);
-- unidade 'caixa' só é convertível para kg se o peso da caixa for informado
ALTER TABLE lote DROP CONSTRAINT IF EXISTS ck_lote_peso_caixa;
ALTER TABLE lote ADD CONSTRAINT ck_lote_peso_caixa
    CHECK ((unidade <> 'caixa' OR peso_caixa_kg IS NOT NULL) AND (peso_caixa_kg IS NULL OR peso_caixa_kg > 0));

ALTER TABLE lote_classificacao DROP CONSTRAINT IF EXISTS ck_classificacao_qtd;
ALTER TABLE lote_classificacao ADD CONSTRAINT ck_classificacao_qtd CHECK (quantidade_kg >= 0);

ALTER TABLE dados_mercado DROP CONSTRAINT IF EXISTS ck_mercado_preco;
ALTER TABLE dados_mercado ADD CONSTRAINT ck_mercado_preco CHECK (preco >= 0);

-- probabilidade sempre de 0 a 1
ALTER TABLE previsao DROP CONSTRAINT IF EXISTS ck_previsao_prob;
ALTER TABLE previsao ADD CONSTRAINT ck_previsao_prob CHECK (probabilidade BETWEEN 0 AND 1);

-- alerta precisa ter origem: uma leitura (faixa) ou uma previsão
ALTER TABLE alerta DROP CONSTRAINT IF EXISTS ck_alerta_origem;
ALTER TABLE alerta ADD CONSTRAINT ck_alerta_origem
    CHECK (fk_previsao_id_previsao IS NOT NULL OR fk_leitura_id_leitura IS NOT NULL);

ALTER TABLE viagem DROP CONSTRAINT IF EXISTS ck_viagem_datas;
ALTER TABLE viagem ADD CONSTRAINT ck_viagem_datas
    CHECK (chegada_prevista IS NULL OR chegada_prevista >= saida);

ALTER TABLE safra DROP CONSTRAINT IF EXISTS ck_safra_datas;
ALTER TABLE safra ADD CONSTRAINT ck_safra_datas CHECK (data_fim IS NULL OR data_fim >= data_inicio);

ALTER TABLE relatorio DROP CONSTRAINT IF EXISTS ck_relatorio_periodo;
ALTER TABLE relatorio ADD CONSTRAINT ck_relatorio_periodo CHECK (fim IS NULL OR inicio IS NULL OR fim >= inicio);

ALTER TABLE padrao_qualidade DROP CONSTRAINT IF EXISTS ck_padrao_brix;
ALTER TABLE padrao_qualidade ADD CONSTRAINT ck_padrao_brix CHECK (brix_min >= 0);

ALTER TABLE propriedade DROP CONSTRAINT IF EXISTS ck_propriedade_geo;
ALTER TABLE propriedade ADD CONSTRAINT ck_propriedade_geo CHECK (
        (latitude  IS NULL OR latitude  BETWEEN -90  AND 90)
    AND (longitude IS NULL OR longitude BETWEEN -180 AND 180)
    AND (area_ha   IS NULL OR area_ha >= 0));

ALTER TABLE porto DROP CONSTRAINT IF EXISTS ck_porto_geo;
ALTER TABLE porto ADD CONSTRAINT ck_porto_geo
    CHECK (latitude BETWEEN -90 AND 90 AND longitude BETWEEN -180 AND 180);

COMMIT;
