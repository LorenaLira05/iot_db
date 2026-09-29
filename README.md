# Banco de Dados ValeSafra

Banco de dados relacional (PostgreSQL) desenvolvido para o projeto **ValeSafra**, focado no monitoramento climático em tempo real, acompanhamento de lotes agrícolas, automação de alertas de contorno de faixa ideal e suporte a análises e previsões de mercado e colheita[cite: 1, 3, 5].

---

## Status do projeto

> **Em desenvolvimento**[cite: 1]

A estrutura do banco de dados foi expandida com **restrições de integridade**, **índices de desempenho**, **gatilhos (triggers) para automação de alertas** e **views otimizadas** para dashboards e relatórios analíticos[cite: 2, 3, 5, 6].

---

## Objetivo

Fornecer uma base de dados consistente e segura para:

* **Gestão de Acesso (RBAC):** Autenticação de usuários, perfis (`Produtor/Exportador`, `Analista de Dados`, `Administrador`) e auditoria/logs[cite: 1, 3, 4].
* **Monitoramento Agrícola:** Cadastro de culturas/frutas, parâmetros ideais (temperatura e umidade) e lotes de produção[cite: 1, 3, 4].
* **Ingestão de Dados IoT:** Integração direta com sensores físicos (como ESP32/DHT11 via ThingSpeak), evitando duplicidade de dados[cite: 1, 2, 4].
* **Automação de Alertas:** Disparo automático de alertas via triggers quando medições saem das faixas operacionais configuradas[cite: 1, 5].
* **Análise e Previsão:** Modelagem para suporte a previsões de mercado, séries temporais climáticas e geração de relatórios[cite: 1, 3, 6].

---

## Estrutura de Arquivos SQL

O repositório está organizado na seguinte ordem de execução recomendada:

| Arquivo | Descrição |
| :--- | :--- |
| `schema.sql` | Criação dos tipos enumerados (`ENUM`) e tabelas principais do sistema[cite: 3]. |
| `restricoes.sql` | Aplicação de chaves únicas (`UNIQUE`), validações de `NOT NULL` e índices de deduplicação[cite: 2]. |
| `seed.sql` | Carga inicial de perfis, frutas, faixas ideais recomendadas (UC Davis / Embrapa) e vínculo com sensores físicos[cite: 4]. |
| `triggers.sql` | Implementação de regras de negócio automatizadas (ex: alertas de variação climática e proteção contra exclusão física de usuários). |
| `views.sql` | Visões materializadas/consultas prontas para consumo por dashboards e módulos analíticos[cite: 6]. |

---

##  Principais Entidades

| Tabela | Descrição |
| :--- | :--- |
| `perfil` | Perfis de acesso do sistema (RBAC)[cite: 1, 3, 4]. |
| `usuario` | Dados e credenciais de acesso dos usuários[cite: 1, 3]. |
| `fruta` | Culturas agrícolas monitoradas (ex: Manga, Uva, Melão)[cite: 1, 3, 4]. |
| `config_parametro` | Configuração das faixas ideais de temperatura e umidade por fruta[cite: 1, 3, 4]. |
| `lote` | Lotes de produção e seu ciclo de vida[cite: 1, 3]. |
| `sensor` | Mapeamento dos dispositivos e canais IoT (ex: ThingSpeak `channel_id`)[cite: 1, 3, 4]. |
| `leitura_climatica` | Histórico de dados coletados (temperatura e umidade) indexados por data e sensor[cite: 1, 3]. |
| `alerta` | Ocorrências registradas automaticamente ao violar as faixas ideais[cite: 1, 3, 5]. |
| `dados_mercado` | Registros de preços e demandas para exportação[cite: 1, 3]. |
| `previsao` / `previsao_climatica` / `previsao_mercado` | Modelagem para integração com modelos preditivos/IA[cite: 1, 3]. |
| `log_acesso` | Logs de auditoria das ações dos usuários[cite: 1, 3]. |
| `relatorio` | Histórico e metadados de relatórios gerados[cite: 1, 3]. |

---

## Funcionalidades e Automações Implementadas

* **Deduplicação da Ingestão ThingSpeak:** Garantida pela constraint `uq_leitura_sensor_entry` (`fk_sensor_id_sensor` + `entry_id_thingspeak`)[cite: 2, 3].
* **Soft Delete para Usuários:** Bloqueio de `DELETE` físico na tabela `usuario` via trigger (`fn_bloquear_delete_usuario`), exigindo o uso de alteração de status (`inativo`)[cite: 3, 5].
* **Trigger Inteligente de Alerta (`trg_alerta_faixa`):** Detecta transições de faixa ideal e gera alertas automaticamente na transição, evitando poluição de notificações repetitivas a cada leitura[cite: 5].
* **Views de Dashboard (`vw_status_sensores` e `vw_leituras_diarias_lote`):** Agregações otimizadas para rápido consumo de interfaces e painéis de controle[cite: 6].

---

## Integração prevista

O banco faz parte de uma arquitetura que poderá integrar diferentes componentes, como:

**Sensores → Coleta de dados → Processamento → Banco de dados → Análise/IA → Alertas → Dashboard**

A estrutura atual serve como base para essa integração e poderá ser modificada conforme os requisitos das demais partes do projeto.

## Tecnologias

* PostgreSQL
* SQL
* Banco de dados relacional

## Contexto

Projeto desenvolvido no contexto acadêmico, como parte de uma solução voltada ao monitoramento e análise de dados relacionados à produção agrícola.
