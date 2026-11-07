# Banco de Dados RAIZ

Banco de dados relacional (PostgreSQL) desenvolvido para o projeto **RAIZ**, focado no monitoramento climático em tempo real, acompanhamento de lotes agrícolas (do campo ao porto), automação de alertas de saída de faixa ideal e suporte a análises e previsões de mercado e colheita.

---

## Status do projeto

> **Em desenvolvimento**

A estrutura já conta com **restrições de integridade**, **índices de desempenho**, **gatilhos (triggers)** para alertas automáticos, **views** para dashboards e relatórios, e modelagem para **logística de exportação** (portos, rotas, viagens) e **cadastro de propriedade/safra**.

---

## Objetivo

Fornecer uma base de dados consistente e segura para:

* **Gestão de Acesso (RBAC):** autenticação de usuários, perfis (`Produtor/Exportador`, `Analista de Dados`, `Administrador`) e auditoria/logs.
* **Monitoramento Agrícola:** cadastro de culturas, variedades, padrões de qualidade, parâmetros ideais (temperatura e umidade por etapa) e lotes de produção.
* **Ingestão de Dados IoT:** integração com sensores físicos (ESP32/DHT11 via ThingSpeak), com deduplicação e registro de leituras rejeitadas. Suporta sensores de **ar** e de **solo**.
* **Automação de Alertas:** alertas disparados por trigger quando uma medição sai da faixa configurada para a etapa do lote.
* **Logística de Exportação:** portos, rotas a partir de Petrolina, viagens por lote e leituras da câmara durante o transporte.
* **Análise e Previsão:** previsões (risco climático, irrigação, prejuízo, janela de colheita), dados de mercado e estimativa de perdas por categoria de risco.

---

## Como criar o banco

Rode os arquivos abaixo **nesta ordem**, parando no primeiro erro:

1. **`schema.sql`**: tipos `ENUM`, todas as tabelas e índices, e atualização de bancos locais antigos.
2. **`implementação_ia.sql`**: tabelas de IA (modelos, chat, previsões detalhadas, parâmetros, cortes de alerta preditivo, cache de parecer).
3. **`auth.sql`**: `refresh_token`, `codigo_2fa_email` e índice de auditoria de login.
4. **`restricoes.sql`**: `NOT NULL`, `DEFAULT`, `UNIQUE` e `CHECK` (roda dentro de uma transação).
5. **`seed.sql`**: perfis, frutas, faixas ideais, lote e sensor reais (canal ThingSpeak `3500765`), portos, fração de perda por risco, cortes do alerta preditivo, sensor de solo simulado e 30 dias de leituras horárias simuladas.
6. **`triggers.sql`**: função `fn_etapa_lote`, alerta automático por transição de faixa, validação lote ↔ variedade e bloqueio de `DELETE` em `usuario`.
7. **`views.sql`**: views do dashboard (dependem de `fn_etapa_lote`, criada em `triggers.sql`).

> **Importação de dados:** importe depois do passo 4 (e do `seed.sql`, que cria os perfis que `usuario` exige), para que cada linha seja validada pelas restrições na entrada.
> **Bancos já criados com a versão anterior:** a revisão mudou chaves estrangeiras (`ON DELETE RESTRICT`), PKs das tabelas de ligação e removeu `alertas_preditivos`. Como ainda não há dados a preservar, **recrie o banco** e rode os scripts na ordem acima.

## Convenções

* **Datas** em `TIMESTAMP` **UTC** (o front converte para `America/Recife`). Exceção intencional: `auth.sql` e `parecer_ia` usam `TIMESTAMPTZ` (expiração de token/cache).
* **Nível / risco / severidade:** sempre o enum `nivel_alerta` (`baixo`, `médio`, `alto`), em alertas, previsões, IA, `perda_risco` e `parametro_prejuizo`.
* **Probabilidade:** sempre de **0 a 1** (`CHECK` em todas as tabelas).
* **Exclusão:** lote, fruta e sensor com histórico não são apagados em cascata (`ON DELETE RESTRICT`). Encerre um lote com `status = 'Finalizado'` e um usuário com `status = 'inativo'`.
* **Sensor de ar** grava `temperatura`/`umidade`; **sensor de solo** grava `temperatura_solo`/`umidade_solo`. Alertas de faixa e médias de ar consideram só sensores `ambiente = 'ar'`.

---

## Principais Entidades

### Acesso e auditoria

| Tabela | Descrição |
| :--- | :--- |
| `perfil` | Perfis de acesso do sistema (RBAC). |
| `usuario` | Dados e credenciais dos usuários (e-mail único sem diferenciar maiúsculas; exclusão física bloqueada). |
| `log_acesso` | Logs de auditoria das ações dos usuários (IP em `INET`). |
| `relatorio` | Histórico e metadados de relatórios gerados. |

### Produção agrícola

| Tabela | Descrição |
| :--- | :--- |
| `propriedade` | Dados da propriedade: localização, área (ha), tipo de solo e moeda de exibição. |
| `safra` | Safras (ex.: `2025/2026`) com período e meta em ton/ha. |
| `fruta` | Culturas monitoradas (Manga, Uva, Melão). |
| `variedade` | Variedades de cada fruta. |
| `padrao_qualidade` | Padrão por variedade: Brix mínimo, calibre mínimo e classe (ex.: Exportação A+). |
| `lote` | Lotes de produção, com quantidade, unidade (`kg`, `caixa`, `t`; `caixa` exige `peso_caixa_kg`), status e destino. Ligado à `fruta` e, opcionalmente, à `variedade`, `safra` e `propriedade`. |
| `lote_classificacao` | Quantidade (kg) do lote por categoria: exportação, mercado interno ou processamento. |
| `config_parametro` | Faixas ideais de temperatura e umidade por **fruta e etapa** (`campo`, `armazenamento`, `transporte`). |

### IoT e alertas

| Tabela | Descrição |
| :--- | :--- |
| `sensor` | Dispositivos e canais IoT (`channel_id` do ThingSpeak), com ambiente `ar` ou `solo` e vínculo a um lote. |
| `leitura_climatica` | Histórico de temperatura e umidade (inclui `temperatura_solo` e `umidade_solo`), indexado por sensor e data. |
| `leitura_rejeitada` | Leituras descartadas na ingestão, guardando o valor bruto e o motivo da rejeição. |
| `alerta` | Ocorrências geradas ao violar as faixas ideais, ligadas à leitura ou à previsão que as originou (uma das duas é obrigatória). Status: `aberto`, `reconhecido`, `resolvido` (`resolvido_em`). |

### Logística de exportação

| Tabela | Descrição |
| :--- | :--- |
| `porto` | Portos de destino (Pecém, Suape, Salvador, Natal) com coordenadas aproximadas. |
| `rota` | Rota de Petrolina até cada porto: distância, duração e traçado em GeoJSON. |
| `viagem` | Viagem de um lote a um porto, com saída, chegada prevista e status (`programada`, `em_transporte`, `concluida`, `cancelada`). |
| `leitura_transporte` | Temperatura, umidade e posição da câmara durante a viagem. |

### Mercado e previsões

| Tabela | Descrição |
| :--- | :--- |
| `dados_mercado` | Preços e demanda de exportação por fruta (com unidade e moeda). |
| `previsao` | Previsão **consolidada** por lote, com tipo (`risco_climatico`, `irrigacao`, `prejuizo`, `janela_colheita`), categoria de risco, valor, unidade, modelo e `detalhes` em JSONB. Alimenta `alerta`, `previsao_climatica` e `previsao_mercado`. |
| `previsao_climatica` / `previsao_mercado` | Ligam uma previsão às leituras climáticas ou aos dados de mercado usados nela (PK composta; some o vínculo se um dos lados for removido). |
| `perda_risco` | Fração de perda associada a cada categoria de risco (`baixo`, `médio`, `alto`). Valores provisórios. |

### Tabelas de IA (`implementação_ia.sql`)

| Tabela | Descrição |
| :--- | :--- |
| `ia_modelo` | Catálogo de modelos (tipo, finalidade, algoritmo, versão, status). |
| `chat_conversa` / `chat_mensagem` | Histórico de conversas do usuário com o assistente. |
| `previsao_prejuizo` | Detalhe tipado de uma previsão de prejuízo (lote e período). |
| `previsao_irrigacao` | Detalhe tipado: necessidade de irrigação e volume de água (mm), com modelo de classificação e de regressão. |
| `risco_climatico` | Detalhe tipado: nível de risco climático por lote e período, com justificativa. |
| `alerta_preditivo` | Alertas gerados por modelos (nível, probabilidade, sensor, lote). Substitui a antiga `alertas_preditivos`. |
| `cortes_alerta_preditivo` | Faixas de probabilidade → nível de alerta e ação recomendada (`limiar_min` inclusivo, `limiar_max` exclusivo). |
| `parametro_prejuizo` / `parametro_irrigacao` | Parâmetros de cálculo por nível de risco / por fruta, com aprovação. |
| `parecer_ia` | Cache do parecer da IA por lote, com expiração. |

`previsao_prejuizo`, `previsao_irrigacao` e `risco_climatico` apontam para a `previsao` consolidada por `fk_previsao_id_previsao`.

### Autenticação (`auth.sql`)

| Tabela | Descrição |
| :--- | :--- |
| `refresh_token` | Sessões com refresh token rotativo (só o hash é salvo). |
| `codigo_2fa_email` | Código de segundo fator por e-mail (só o HMAC é salvo). |

--- | :--- |
| `ia_modelo` | Catálogo de modelos (tipo, finalidade, algoritmo, versão, status). |
| `chat_conversa` / `chat_mensagem` | Histórico de conversas do usuário com o assistente. |
| `previsao_prejuizo` | Prejuízo estimado por lote e período. |
| `alerta_preditivo` | Alertas gerados por modelos, com classificação e probabilidade. |
| `previsao_irrigacao` | Necessidade de irrigação e volume de água (mm), com modelo de classificação e de regressão. |
| `risco_climatico` | Nível de risco climático por lote e período, com justificativa. |

---

## Funcionalidades e Automações

* **Deduplicação da ingestão ThingSpeak:** `uq_leitura_sensor_entry` (`fk_sensor_id_sensor` + `entry_id_thingspeak`). Leituras rejeitadas também são deduplicadas (`uq_rejeitada_sensor_entry`).
* **Faixa ideal por etapa do lote:** `fn_etapa_lote` converte o status do lote em etapa (`Em produção` e `Pronto para colheita` → `campo`; `Em transporte` → `transporte`; demais → `armazenamento`). Se não houver faixa própria para `transporte`, usa a de `armazenamento`. O seed só cadastra faixas de `armazenamento`.
* **Alerta por transição (`trg_alerta_faixa`):** cria um alerta de nível `médio` (`temperatura_fora_faixa` ou `umidade_fora_faixa`) apenas quando a leitura **sai** da faixa em relação à leitura anterior do mesmo sensor, evitando alertas repetidos a cada leitura. Sensores sem lote, ou etapas sem faixa configurada, não geram alerta.
* **Soft delete de usuários:** `DELETE` físico em `usuario` é bloqueado por trigger (`fn_bloquear_delete_usuario`); use `status = 'inativo'`.
* **Validações (`CHECK`):** ambiente do sensor, etapa da faixa, tipo de previsão, unidade do lote (e peso da caixa), status da viagem e do alerta, categoria da classificação, `fracao_perda` e probabilidade entre 0 e 1, umidade entre 0 e 100, `temp_min < temp_max`, datas coerentes (viagem, safra, relatório). Leitura com valor fora da faixa física deve ir para `leitura_rejeitada`.
* **Dados simulados:** o seed cria o sensor `Solo simulado` no mesmo lote do sensor real e gera leituras horárias dos últimos 30 dias. Rodar de novo só completa o que falta.

### Views

| View | Uso |
| :--- | :--- |
| `vw_status_sensores` | Última leitura de cada sensor, para o painel de status. |
| `vw_leitura_hora` | Médias por hora e sensor, para gráficos. |
| `vw_leituras_diarias_lote` | Média, mínimo e máximo diários por lote (visão do analista), no dia local de Recife; ar e solo em colunas separadas. |
| `vw_lote_classificacao_conferencia` | Compara a soma de `lote_classificacao` (kg) com a quantidade do lote convertida para kg. |
| `vw_lote_saude` | Percentual de leituras das últimas 24 h dentro da faixa da etapa (sensores de `ar`), com selo **Saudável** (≥ 90%), **Atenção** (≥ 70%) ou **Crítico**. Limites provisórios. |

---

## Pontos de atenção

* Os limites do selo de saúde e as frações de perda por risco são **provisórios** e vão ser definidos pelo grupo.
* `perda_risco` e `parametro_prejuizo` guardam ambos uma fração de perda por nível de risco. **Definir qual é a fonte oficial** e remover a outra.
* O `seed.sql` só cadastra faixas de `armazenamento`. Um lote `Em produção` (etapa `campo`) **não gera alerta nem aparece em `vw_lote_saude`** até existir uma faixa de `campo` em `config_parametro`.
* O vocabulário de `chat_mensagem.tipo` e `ia_modelo.status` ainda é livre (definir os valores e adicionar `CHECK`).
* As coordenadas dos portos são aproximadas; confira antes de usar em cálculo de rota. A tabela `rota` não tem seed.
* Colunas renomeadas nesta revisão (ajustar consultas do backend): `dados_mercado.data_refencia` → `data_referencia`, `previsao.modelo_ultilizado` → `modelo_utilizado`.

---

## Integração prevista

O banco faz parte de uma arquitetura que poderá integrar diferentes componentes:

**Sensores → Coleta de dados → Processamento → Banco de dados → Análise/IA → Alertas → Dashboard**

A estrutura atual serve como base para essa integração e poderá ser modificada conforme os requisitos das demais partes do projeto.

## Tecnologias

* PostgreSQL
* SQL (PL/pgSQL para funções e triggers)
* ThingSpeak e ESP32/DHT11 e sensor de umidade do solo (fonte dos dados IoT)

## Contexto

Projeto desenvolvido no contexto acadêmico, como parte de uma solução voltada ao monitoramento e análise de dados relacionados à produção agrícola.
