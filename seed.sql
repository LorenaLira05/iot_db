-- seed.sql
-- Dados iniciais do banco. Pode rodar mais de uma vez: só insere o que não existe
-- e atualiza as faixas ideais de config_parametro.
-- Ordem: schema -> restricoes.sql -> seed.sql
-- Ajuste os valores da seção CONFIG antes de rodar.

DO $$
DECLARE
    -- ===== CONFIG (fruta/lote/sensor do sensor físico real) =====
    v_fruta_nome   TEXT    := 'Manga';
    v_lote_qtd     INTEGER := 1000;
    v_lote_destino TEXT    := 'Exportação';
    v_sensor_nome  TEXT    := 'DHT11 ESP32-C3';
    v_sensor_local TEXT    := 'Bancada';
    v_channel_id   INTEGER := 3500765;
    -- ============================================================
    v_fruta_id INTEGER;
    v_lote_id  INTEGER;
BEGIN
    -- Perfis (RBAC)
    INSERT INTO perfil (nome, descricao)
    SELECT p.nome, p.descricao
    FROM (VALUES
        ('Produtor/Exportador', 'Irrigação e logística fria'),
        ('Analista de Dados',   'Histórico, agregações e previsões'),
        ('Administrador',       'Usuários, parâmetros e auditoria')
    ) AS p(nome, descricao)
    WHERE NOT EXISTS (SELECT 1 FROM perfil x WHERE x.nome = p.nome);

    -- Frutas
    INSERT INTO fruta (nome)
    SELECT f.nome
    FROM (VALUES ('Manga'), ('Uva'), ('Melão')) AS f(nome)
    WHERE NOT EXISTS (SELECT 1 FROM fruta x WHERE x.nome = f.nome);

    -- Faixas ideais (armazenamento refrigerado e transporte pós-colheita)
    -- Fontes: UC Davis Postharvest Research and Extension Center (manga, uva,
    -- tabela de compatibilidade), National Mango Board, artigos Embrapa/SciELO (melão amarelo).
    -- Mudar um valor aqui e rodar o seed de novo atualiza a linha.
    INSERT INTO config_parametro
        (fk_fruta_id_fruta, temp_min, temp_max, umidade_min, umidade_max)
    SELECT f.id_fruta, p.tmin, p.tmax, p.umin, p.umax
    FROM (VALUES
        ('Manga',  10.0, 13.0, 90.0, 95.0),
        ('Uva',    -0.5,  2.0, 90.0, 95.0),
        ('Melão',  10.0, 12.0, 85.0, 95.0)
    ) AS p(nome, tmin, tmax, umin, umax)
    JOIN fruta f ON f.nome = p.nome
    ON CONFLICT (fk_fruta_id_fruta) DO UPDATE
        SET temp_min    = EXCLUDED.temp_min,
            temp_max    = EXCLUDED.temp_max,
            umidade_min = EXCLUDED.umidade_min,
            umidade_max = EXCLUDED.umidade_max;

    -- Lote do sensor real
    SELECT id_fruta INTO v_fruta_id FROM fruta WHERE nome = v_fruta_nome LIMIT 1;

    SELECT id_lote INTO v_lote_id
    FROM lote
    WHERE fk_fruta_id_fruta = v_fruta_id AND destino = v_lote_destino
    LIMIT 1;
    IF v_lote_id IS NULL THEN
        INSERT INTO lote (data_colheita, quantidade, status, destino, fk_fruta_id_fruta)
        VALUES (NULL, v_lote_qtd, 'Em produção', v_lote_destino, v_fruta_id)
        RETURNING id_lote INTO v_lote_id;
    END IF;

    -- Sensor (identificado pelo channel_id do ThingSpeak)
    IF NOT EXISTS (SELECT 1 FROM sensor WHERE channel_id = v_channel_id) THEN
        INSERT INTO sensor (nome, tipo_sensor, localizacao, channel_id, status, fk_lote_id_lote)
        VALUES (v_sensor_nome, 'DHT11', v_sensor_local, v_channel_id, 'ativo', v_lote_id);
    END IF;
END $$;

-- Conferência
SELECT id_sensor, nome, channel_id, fk_lote_id_lote FROM sensor;
SELECT f.nome, c.temp_min, c.temp_max, c.umidade_min, c.umidade_max
FROM config_parametro c JOIN fruta f ON f.id_fruta = c.fk_fruta_id_fruta
ORDER BY f.nome;
