-- triggers.sql
-- Funções e gatilhos. Idempotente. Roda depois do seed.sql.

CREATE OR REPLACE FUNCTION fn_etapa_lote(p_status status_lote)
RETURNS VARCHAR AS $$
    SELECT CASE p_status
             WHEN 'Em produção'          THEN 'campo'
             WHEN 'Pronto para colheita' THEN 'campo'
             WHEN 'Em transporte'        THEN 'transporte'
             ELSE 'armazenamento'
           END;
$$ LANGUAGE sql IMMUTABLE;

CREATE OR REPLACE FUNCTION fn_alerta_faixa()
RETURNS TRIGGER AS $$
DECLARE
    v_cfg config_parametro%ROWTYPE;
    v_ant RECORD;
    v_etapa         VARCHAR;
    v_agora         TIMESTAMP := (now() AT TIME ZONE 'UTC');  -- UTC; o front converte para America/Recife
    v_temp_fora     BOOLEAN;
    v_umid_fora     BOOLEAN;
    v_temp_fora_ant BOOLEAN := FALSE;
    v_umid_fora_ant BOOLEAN := FALSE;
BEGIN
    -- etapa do lote do sensor (só sensores de AR: a faixa ideal é de temperatura/umidade do ar;
    -- sensor de solo mede o solo, que tem outra faixa ideal, e por isso não passa por esta função)
    SELECT fn_etapa_lote(l.status) INTO v_etapa
    FROM sensor s
    JOIN lote l ON l.id_lote = s.fk_lote_id_lote
    WHERE s.id_sensor = NEW.fk_sensor_id_sensor
      AND s.ambiente = 'ar';

    IF v_etapa IS NULL THEN
        RETURN NEW;  -- sem sensor de ar, lote ou status
    END IF;

    -- faixa da fruta para a etapa. Transporte cai em armazenamento se não houver faixa própria.
    SELECT c.* INTO v_cfg
    FROM sensor s
    JOIN lote l ON l.id_lote = s.fk_lote_id_lote
    JOIN config_parametro c ON c.fk_fruta_id_fruta = l.fk_fruta_id_fruta
    WHERE s.id_sensor = NEW.fk_sensor_id_sensor
      AND (c.etapa = v_etapa
           OR (v_etapa = 'transporte' AND c.etapa = 'armazenamento'))
    ORDER BY (c.etapa = v_etapa) DESC
    LIMIT 1;

    IF NOT FOUND THEN
        RETURN NEW;  -- sem faixa configurada para a etapa: não alerta
    END IF;

    v_temp_fora := COALESCE(
        NEW.temperatura < v_cfg.temp_min OR NEW.temperatura > v_cfg.temp_max, FALSE);
    v_umid_fora := COALESCE(
        NEW.umidade < v_cfg.umidade_min OR NEW.umidade > v_cfg.umidade_max, FALSE);

    SELECT temperatura, umidade INTO v_ant
    FROM leitura_climatica
    WHERE fk_sensor_id_sensor = NEW.fk_sensor_id_sensor
      AND (data_hora, id_leitura) < (NEW.data_hora, NEW.id_leitura)
    ORDER BY data_hora DESC, id_leitura DESC
    LIMIT 1;

    IF FOUND THEN
        v_temp_fora_ant := COALESCE(
            v_ant.temperatura < v_cfg.temp_min OR v_ant.temperatura > v_cfg.temp_max, FALSE);
        v_umid_fora_ant := COALESCE(
            v_ant.umidade < v_cfg.umidade_min OR v_ant.umidade > v_cfg.umidade_max, FALSE);
    END IF;

    IF v_temp_fora AND NOT v_temp_fora_ant THEN
        INSERT INTO alerta (tipo_alerta, descricao, nivel, data_hora, status, fk_leitura_id_leitura)
        VALUES ('temperatura_fora_faixa',
                left(format('Temperatura %s°C fora da faixa (%s a %s°C)',
                            NEW.temperatura, v_cfg.temp_min, v_cfg.temp_max), 150),
                'médio', v_agora, 'aberto', NEW.id_leitura);
    END IF;

    IF v_umid_fora AND NOT v_umid_fora_ant THEN
        INSERT INTO alerta (tipo_alerta, descricao, nivel, data_hora, status, fk_leitura_id_leitura)
        VALUES ('umidade_fora_faixa',
                left(format('Umidade %s%% fora da faixa (%s a %s%%)',
                            NEW.umidade, v_cfg.umidade_min, v_cfg.umidade_max), 150),
                'médio', v_agora, 'aberto', NEW.id_leitura);
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_alerta_faixa ON leitura_climatica;
CREATE TRIGGER trg_alerta_faixa
AFTER INSERT ON leitura_climatica
FOR EACH ROW EXECUTE FUNCTION fn_alerta_faixa();

CREATE OR REPLACE FUNCTION fn_bloquear_delete_usuario()
RETURNS TRIGGER AS $$
BEGIN
    RAISE EXCEPTION 'Exclusão física de usuário não permitida. Use status = ''inativo''.';
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_bloquear_delete_usuario ON usuario;
CREATE TRIGGER trg_bloquear_delete_usuario
BEFORE DELETE ON usuario
FOR EACH ROW EXECUTE FUNCTION fn_bloquear_delete_usuario();

-- A variedade do lote precisa pertencer à fruta do lote
CREATE OR REPLACE FUNCTION fn_lote_variedade_coerente()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.fk_variedade_id_variedade IS NOT NULL AND NOT EXISTS (
        SELECT 1 FROM variedade v
        WHERE v.id_variedade = NEW.fk_variedade_id_variedade
          AND v.fk_fruta_id_fruta = NEW.fk_fruta_id_fruta
    ) THEN
        RAISE EXCEPTION 'Variedade % não pertence à fruta % do lote',
            NEW.fk_variedade_id_variedade, NEW.fk_fruta_id_fruta;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_lote_variedade_coerente ON lote;
CREATE TRIGGER trg_lote_variedade_coerente
BEFORE INSERT OR UPDATE OF fk_fruta_id_fruta, fk_variedade_id_variedade ON lote
FOR EACH ROW EXECUTE FUNCTION fn_lote_variedade_coerente();

-- Sensor de SOLO só mede umidade do solo. A ingestão grava toda leitura em `umidade`;
-- aqui o valor também é copiado para `umidade_solo` (coluna oficial do solo). `umidade` continua
-- preenchida porque o backend (previsao_repo.ultima_leitura) e o dashboard ainda leem daí;
-- as médias de ar nas views ignoram sensor de solo.
CREATE OR REPLACE FUNCTION fn_leitura_solo()
RETURNS TRIGGER AS $$
BEGIN
    IF EXISTS (SELECT 1 FROM sensor s
               WHERE s.id_sensor = NEW.fk_sensor_id_sensor AND s.ambiente = 'solo') THEN
        IF NEW.umidade_solo IS NULL THEN
            NEW.umidade_solo := NEW.umidade;
        END IF;
        IF NEW.umidade IS NULL THEN
            NEW.umidade := NEW.umidade_solo;
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_leitura_solo ON leitura_climatica;
CREATE TRIGGER trg_leitura_solo
BEFORE INSERT ON leitura_climatica
FOR EACH ROW EXECUTE FUNCTION fn_leitura_solo();
