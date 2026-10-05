CREATE OR REPLACE FUNCTION fn_alerta_faixa()
RETURNS TRIGGER AS $$
DECLARE
    v_cfg config_parametro%ROWTYPE;
    v_ant RECORD;
    v_etapa         VARCHAR;
    v_agora         TIMESTAMP := (now() AT TIME ZONE 'America/Recife');
    v_temp_fora     BOOLEAN;
    v_umid_fora     BOOLEAN;
    v_temp_fora_ant BOOLEAN := FALSE;
    v_umid_fora_ant BOOLEAN := FALSE;
BEGIN
    -- etapa do lote do sensor
    SELECT fn_etapa_lote(l.status) INTO v_etapa
    FROM sensor s
    JOIN lote l ON l.id_lote = s.fk_lote_id_lote
    WHERE s.id_sensor = NEW.fk_sensor_id_sensor;

    IF v_etapa IS NULL THEN
        RETURN NEW;  -- sem sensor, lote ou status
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
      AND id_leitura < NEW.id_leitura
    ORDER BY id_leitura DESC
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

CREATE OR REPLACE FUNCTION fn_etapa_lote(p_status status_lote)
RETURNS VARCHAR AS $$
    SELECT CASE p_status
             WHEN 'Em produção'          THEN 'campo'
             WHEN 'Pronto para colheita' THEN 'campo'
             WHEN 'Em transporte'        THEN 'transporte'
             ELSE 'armazenamento'
           END;
$$ LANGUAGE sql IMMUTABLE;

CREATE TRIGGER trg_alerta_faixa
AFTER INSERT ON leitura_climatica
FOR EACH ROW EXECUTE FUNCTION fn_alerta_faixa();

CREATE OR REPLACE FUNCTION fn_bloquear_delete_usuario()
RETURNS TRIGGER AS $$
BEGIN
    RAISE EXCEPTION 'Exclusão física de usuário não permitida. Use status = ''inativo''.';
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_bloquear_delete_usuario
BEFORE DELETE ON usuario
FOR EACH ROW EXECUTE FUNCTION fn_bloquear_delete_usuario();
