-- views.sql
-- Rodar por último (vw_lote_saude usa fn_etapa_lote, criada em triggers.sql).
-- Ar: temperatura/umidade, só de sensores com ambiente = 'ar'.
-- Solo: umidade_solo (não existe temperatura do solo); trg_leitura_solo move a umidade do sensor de solo para ela.
-- Colunas novas ficam no FIM de cada view.

-- View para o dashboard: última leitura de cada sensor
CREATE OR REPLACE VIEW vw_status_sensores AS
SELECT s.id_sensor, s.nome, s.channel_id, s.status,
       s.fk_lote_id_lote AS id_lote,
       u.data_hora AS ultima_leitura, u.temperatura, u.umidade,
       s.ambiente, u.umidade_solo
FROM sensor s
LEFT JOIN LATERAL (
    SELECT l.data_hora, l.temperatura, l.umidade, l.umidade_solo
    FROM leitura_climatica l
    WHERE l.fk_sensor_id_sensor = s.id_sensor
    ORDER BY l.data_hora DESC
    LIMIT 1
) u ON TRUE;

-- Analista: agregações diárias por lote. O dia é o dia LOCAL (America/Recife), não o dia UTC.
CREATE OR REPLACE VIEW vw_leituras_diarias_lote AS
SELECT s.fk_lote_id_lote AS id_lote,
       ((l.data_hora AT TIME ZONE 'UTC') AT TIME ZONE 'America/Recife')::date AS dia,
       avg(l.temperatura) FILTER (WHERE s.ambiente = 'ar') AS temp_media,
       min(l.temperatura) FILTER (WHERE s.ambiente = 'ar') AS temp_min,
       max(l.temperatura) FILTER (WHERE s.ambiente = 'ar') AS temp_max,
       avg(l.umidade)     FILTER (WHERE s.ambiente = 'ar') AS umid_media,
       min(l.umidade)     FILTER (WHERE s.ambiente = 'ar') AS umid_min,
       max(l.umidade)     FILTER (WHERE s.ambiente = 'ar') AS umid_max,
       count(*)           FILTER (WHERE s.ambiente = 'ar') AS qtd_leituras,
       avg(l.umidade_solo) AS umid_solo_media,
       min(l.umidade_solo) AS umid_solo_min,
       max(l.umidade_solo) AS umid_solo_max,
       count(*)           FILTER (WHERE s.ambiente = 'solo') AS qtd_leituras_solo
FROM leitura_climatica l
JOIN sensor s ON s.id_sensor = l.fk_sensor_id_sensor
GROUP BY 1, 2;

-- Gráficos do dashboard: médias por hora (troque 'hour' por 'day' para diário)
CREATE OR REPLACE VIEW vw_leitura_hora AS
SELECT fk_sensor_id_sensor,
       date_trunc('hour', data_hora) AS hora,
       ROUND(AVG(temperatura)::numeric, 2) AS temperatura_media,
       ROUND(AVG(umidade), 2)              AS umidade_media,
       COUNT(*)                            AS n_leituras,
       ROUND(AVG(umidade_solo), 2)         AS umidade_solo_media
FROM leitura_climatica
GROUP BY fk_sensor_id_sensor, date_trunc('hour', data_hora);

-- Selo do lote: % de leituras das últimas 24 h dentro da faixa da etapa (só sensores de ar)
CREATE OR REPLACE VIEW vw_lote_saude AS
WITH pct AS (
  SELECT l.id_lote,
         COUNT(*) AS n,
         100.0 * COUNT(*) FILTER (
           WHERE r.temperatura BETWEEN c.temp_min AND c.temp_max
             AND r.umidade     BETWEEN c.umidade_min AND c.umidade_max
         ) / NULLIF(COUNT(*), 0) AS pct_na_faixa
  FROM lote l
  JOIN sensor s ON s.fk_lote_id_lote = l.id_lote
               AND s.ambiente = 'ar'
  JOIN leitura_climatica r ON r.fk_sensor_id_sensor = s.id_sensor
                          AND r.data_hora >= (now() AT TIME ZONE 'UTC') - interval '24 hours'
  JOIN LATERAL (
    SELECT c.* FROM config_parametro c
    WHERE c.fk_fruta_id_fruta = l.fk_fruta_id_fruta
      AND (c.etapa = fn_etapa_lote(l.status)
           OR (fn_etapa_lote(l.status) = 'transporte' AND c.etapa = 'armazenamento'))
    ORDER BY (c.etapa = fn_etapa_lote(l.status)) DESC
    LIMIT 1
  ) c ON TRUE
  GROUP BY l.id_lote
)
SELECT id_lote, n AS n_leituras, ROUND(pct_na_faixa, 1) AS pct_na_faixa,
       CASE WHEN pct_na_faixa >= 90 THEN 'Saudável'   -- limites provisórios: o grupo define
            WHEN pct_na_faixa >= 70 THEN 'Atenção'
            ELSE 'Crítico' END AS selo
FROM pct;

-- Conferência da classificação: a soma por categoria (kg) não deve passar da quantidade do lote.
-- 'caixa' só converte para kg se peso_caixa_kg estiver preenchido (CHECK em restricoes.sql).
CREATE OR REPLACE VIEW vw_lote_classificacao_conferencia AS
WITH base AS (
  SELECT l.id_lote,
         CASE l.unidade
              WHEN 'kg'    THEN l.quantidade
              WHEN 't'     THEN l.quantidade * 1000
              WHEN 'caixa' THEN l.quantidade * l.peso_caixa_kg
         END AS quantidade_kg_lote,
         COALESCE(SUM(c.quantidade_kg), 0) AS soma_classificada_kg
  FROM lote l
  LEFT JOIN lote_classificacao c ON c.fk_lote_id_lote = l.id_lote
  GROUP BY l.id_lote
)
SELECT id_lote, quantidade_kg_lote, soma_classificada_kg,
       quantidade_kg_lote - soma_classificada_kg AS diferenca_kg,
       (soma_classificada_kg > quantidade_kg_lote) AS excede_lote
FROM base;

CREATE OR REPLACE VIEW dado_mercado AS
SELECT m.id_mercado,
       m.fk_fruta_id_fruta,
       CASE lower(m.unidade) WHEN 'kg' THEN m.preco
                             WHEN 't'  THEN m.preco / 1000 END AS preco_kg,
       (m.data_refencia::timestamp AT TIME ZONE 'UTC')          AS data_hora,
       m.moeda
FROM dados_mercado m
WHERE m.moeda = 'BRL'
  AND lower(m.unidade) IN ('kg', 't')
  AND m.data_refencia IS NOT NULL;
